#include <LSP/MIDI/Sequencer.hpp>
#include <LSP/MIDI/MessageReceiver.hpp>
#include <LSP/MIDI/Messages/BasicMessage.hpp>
#include <LSP/MIDI/Messages/SysExMessage.hpp>
#include <LSP/Threading/Thread.hpp>
#include <LSP/Threading/EventSignal.hpp>
#include <LSP/Base/Logging.hpp>

using namespace LSP;
using namespace LSP::MIDI;

// 参考URL :
//  http://eternalwindows.jp/winmm/midi/midi00.html
//  https://www.g200kg.com/jp/docs/dic/midi.html
//  https://www.cs.cmu.edu/~music/cmsip/readings/Standard-MIDI-file-format-updated.pdf
//   http://lib.roland.co.jp/support/jp/manuals/res/1810481/SC-8850_j8.pdf

namespace 
{

// ビッグエンディアンの値を読み出します
template<
	std::integral T
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


Sequencer::Sequencer(MessageReceiver& receiver)
	: mReceiver(receiver)
	, mPlayThreadAbortFlag(false)
	, mPlayThreadPausingFlag(false)
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
	mPlayThreadPausingFlag = false;
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
void Sequencer::pause()
{
	mPlayThreadPausingFlag = true;
}

void Sequencer::resume()
{
	mPlayThreadPausingFlag = false;
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

bool Sequencer::isPausing()const
{
	return mPlayThreadPausingFlag;
}

void Sequencer::reset(SystemType type)
{
	std::shared_ptr<Message> msg;
	switch (type) {
	case SystemType::GM1:
		msg = std::make_shared<Messages::SysExMessage>(std::vector<uint8_t>{ 0x7E, 0x7F, 0x09, 0x01 });
		break;
	case SystemType::GM2:
		msg = std::make_shared<Messages::SysExMessage>(std::vector<uint8_t>{ 0x7E, 0x7F, 0x09, 0x03 });
		break;
	case SystemType::GS:
		msg = std::make_shared<Messages::SysExMessage>(std::vector<uint8_t>{ 0x7E, 0x7F, 0x09, 0x02 });
		break;
	}
	if (msg) {
		mReceiver.onMidiMessageReceived(clock::time_point::min(), msg);
	}
}

void Sequencer::playThreadMain(const Body& smfBody)
{
	static constexpr std::chrono::milliseconds max_sleep_duration{ 100 };

	std::optional<clock::time_point> pause_begin_time;
	auto start_time = clock::now();
	auto next_message_iter = smfBody.cbegin();

	while (true) {
		if(mPlayThreadAbortFlag) break;

		// 再生中断が指示された場合、再開までメッセージは処理しない
		if(mPlayThreadPausingFlag) {
			if(!pause_begin_time.has_value()) {
				pause_begin_time = clock::now();
			}

			// 待機中は オールサウンドオフを継続的に投げて音を切っておく
			// TODO 若干非効率なため、もうすこし効率的な方法を探す
			for(uint8_t ch = 0; ch <= 0x0F; ++ch) {
				auto msg = std::make_shared< MIDI::Messages::ControlChange>(ch, 120, 0); // All Sound Off
				mReceiver.onMidiMessageReceived(clock::time_point::min(), msg);
			}
			std::this_thread::sleep_for(max_sleep_duration);
			continue;
		}
		// 再生が再開された場合、中断された時刻からの経過時間を現在の再生位置から巻き戻す
		if(pause_begin_time.has_value()) {
			start_time += clock::now() - pause_begin_time.value();
			pause_begin_time.reset();
		}

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
			mReceiver.onMidiMessageReceived(msg_time,  msg);
			++next_message_iter;
		}

		// 処理すべきメッセージが無くなった場合、停止
		if(next_message_iter == smfBody.cend()) break;

		// 次のメッセージまで待機
		auto max_sleep_until = clock::now() + max_sleep_duration;
		std::this_thread::sleep_until(std::min(next_message_time, max_sleep_until));
	}
}