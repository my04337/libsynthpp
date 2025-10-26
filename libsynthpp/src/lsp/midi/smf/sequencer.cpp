#include <lsp/midi/smf/sequencer.hpp>
#include <lsp/midi/message_receiver.hpp>
#include <lsp/midi/messages/sysex_message.hpp>
#include <lsp/util/thread_priority.hpp>

using namespace lsp;
using namespace lsp::midi::smf;

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
	require(bytes <= sizeof(T));
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

	std::promise<void> ready_promise;
	auto ready_future = ready_promise.get_future();
	mPlayThread = std::thread([this, &ready_promise]()
	{
		lsp::this_thread::set_priority(ThreadPriority::AboveNormal);

		auto body = mSmfBody; // copy
		ready_promise.set_value();
		playThreadMain(body);
		mPlayThreadAbortFlag = true;
	});
	ready_future.wait();
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
void Sequencer::reset(SystemType type)
{
	// TODO System Mode Set 1/2には非対応
	std::shared_ptr<Message> msg;
	if(type.isOnlyGM1()) {
		msg = std::make_shared<messages::SysExMessage>(std::vector<uint8_t>{ 0x7E, 0x7F, 0x09, 0x01 });
	} else if (type.isGM2()) {
		msg = std::make_shared<messages::SysExMessage>(std::vector<uint8_t>{ 0x7E, 0x7F, 0x09, 0x03 });
	} else if (type.isOnlyGS()) {
		msg = std::make_shared<messages::SysExMessage>(std::vector<uint8_t>{ 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41 });
	} else if (type.isSystemModeSet1()) {
		msg = std::make_shared<messages::SysExMessage>(std::vector<uint8_t>{ 0x41, 0x10, 0x42, 0x12, 0x00, 0x00, 0x7F, 0x00, 0x01 });
	} else if (type.isSystemModeSet2()) {
		msg = std::make_shared<messages::SysExMessage>(std::vector<uint8_t>{ 0x41, 0x10, 0x42, 0x12, 0x00, 0x00, 0x7F, 0x01, 0x00 });
	} else if (type.isXG()) {
		msg = std::make_shared<messages::SysExMessage>(std::vector<uint8_t>{ 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00 });
	} else {
		lsp_check(false); // Unsupported SystemType
	}
	if (msg) {
		mReceiver.onMidiMessageReceived(std::chrono::steady_clock::time_point::min(), msg);
	}
}

void Sequencer::playThreadMain(const Body& smfBody)
{
	static constexpr std::chrono::milliseconds max_sleep_duration{ 100 };

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