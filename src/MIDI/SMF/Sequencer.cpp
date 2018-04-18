#include <LSP/MIDI/SMF/Sequencer.hpp>
#include <LSP/Threading/Thread.hpp>
#include <LSP/Threading/EventSignal.hpp>
#include <LSP/Base/Logging.hpp>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::MIDI::SMF;

// 参考URL :
//  http://eternalwindows.jp/winmm/midi/midi00.html
//  https://www.g200kg.com/jp/docs/dic/midi.html
//  https://www.cs.cmu.edu/~music/cmsip/readings/Standard-MIDI-file-format-updated.pdf

namespace 
{

// ビッグエンディアンの値を読み出します
template<
	typename T,
	class = std::enable_if_t<std::is_arithmetic_v<T>>
>
std::optional<T> read_big(std::istream& s, size_t bytes = sizeof(T)) 
{
	// TODO リトルエンディアンでの実行前提
	// TODO アラインメントが適切ではない可能性あり
	lsp_assert(bytes <= sizeof(T))
	std::array<uint8_t, sizeof(T)> buff = {0};
	for (size_t i = 0; i < bytes; ++i) {
		int b = s.get();
		if(b < 0) return {};
		buff[bytes-i-1] = static_cast<uint8_t>(b);
	}
	return *reinterpret_cast<T*>(&buff[0]);
}

// 1バイトを読み出します
std::optional<uint8_t> read_byte(std::istream& s) 
{
	// TODO リトルエンディアンでの実行前提
	// TODO アラインメントが適切ではない可能性あり

	int b = s.get();
	if(b < 0) return {};
	return static_cast<uint8_t>(b);
}

// デルタタイム(可変長数値)を読み出します
std::optional<uint32_t> read_variable(std::istream& s) 
{
	uint32_t ret = 0;
	int read_bytes = 0;
	while (true) {
		// 最大でも4バイトで構成される
		if(read_bytes >= 4) return {}; 

		int b = s.get();
		++read_bytes;
		if(b < 0) return {}; // EOF or error

		// 数値として使う値は7bit単位で格納されている
		ret <<= 7;
		ret += static_cast<uint8_t>(b & 0x7F);

		// 最上位桁が0ならば、この桁で終わり
		if((b & 0x80) == 0) break;
	}
	return ret;
}

}


Sequencer::Sequencer(Synthesizer::ToneGenerator& gen)
	: mToneGenerator(gen)
	, mPlayThreadAbortFlag(false)
{
}

Sequencer::~Sequencer()
{
	stop();
}

void Sequencer::load(Body&& body)
{
	stop();
	mSmfBody = std::move(body);
}
void Sequencer::start()
{
	stop();
	mPlayThreadAbortFlag = false;

	Threading::EventSignal sig;
	mPlayThread = std::thread([this, &sig]
	{
		auto body = mSmfBody; // copy
		sig.set();
		playThreadMain(body);
		mPlayThreadAbortFlag = true;
	});
	Threading::setThreadPriority(mPlayThread, Threading::Priority::AboveNormal);
	sig.wait();
}

void Sequencer::stop() 
{
	if(mPlayThread.joinable()) {
		mPlayThreadAbortFlag = true;
		mPlayThread.join();
	}
}
bool Sequencer::isPlaying()const
{
	return !mPlayThreadAbortFlag;
}

void Sequencer::playThreadMain(const Body& smfBody)
{
	using clock = std::chrono::steady_clock;
	auto start_time = clock::now();
	auto next_message_iter = smfBody.cbegin();

	while (true) {
		if(mPlayThreadAbortFlag) break;

		// 処理時間が現在より手前のメッセージを処理する
		clock::time_point next_message_time;
		while (next_message_iter != smfBody.cend()) {
			auto now_time = clock::now(); // 処理中にも現在時間は変わる
			auto msg_time = start_time + next_message_iter->first;
			auto& msg = next_message_iter->second;

			if (msg_time >= now_time) {
				next_message_time = msg_time;
				break;
			}

			msg->play(mToneGenerator);
			++next_message_iter;
		}

		// 処理すべきメッセージが無くなった場合、停止
		if(next_message_iter == smfBody.cend()) break;

		// 次のメッセージまで待機
		std::this_thread::sleep_until(next_message_time);
	}
}