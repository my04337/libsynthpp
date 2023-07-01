/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

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
{
}

Sequencer::~Sequencer()
{
	stop();
}

void Sequencer::load(juce::MidiFile&& midiFile)
{
	stop();

	// 時刻をMIDIタイムスタンプ(デルタタイムベース?)から先頭からの秒に変換
	midiFile.convertTimestampTicksToSeconds();

	// シーケンサで扱うため全チャネルを統合
	juce::MidiMessageSequence sequence;
	for(int trackNo = 0; trackNo < midiFile.getNumTracks(); ++trackNo) {
		sequence.addSequence(*midiFile.getTrack(trackNo), 0.0);
	}

	// ロード完了
	mSequence = std::move(sequence);
}
void Sequencer::start()
{
	stop();

	std::promise<void> ready_promise;
	auto ready_future = ready_promise.get_future();
	mPlayThread = std::jthread([this, &ready_promise]()
	{
		lsp::this_thread::set_priority(ThreadPriority::AboveNormal);

		auto body = mSequence; // copy
		ready_promise.set_value();
		playThreadMain(mPlayThread.get_stop_token(), body);
	});
	ready_future.wait();
}

void Sequencer::stop() 
{
	if(mPlayThread.joinable()) {
		mPlayThread.request_stop();
		mPlayThread.join();
	}
}
bool Sequencer::isPlaying()const
{
	return mPlayThread.joinable() && !mPlayThread.get_stop_token().stop_requested();
}

void Sequencer::playThreadMain(std::stop_token stopToken, const juce::MidiMessageSequence& sequence)
{
	static constexpr std::chrono::milliseconds max_sleep_duration{ 100 };

	auto start_time = clock::now();
	auto next_message_iter = sequence.begin();

	while (true) {
		if(stopToken.stop_requested()) break;

		// 処理時間が現在より手前のメッセージを処理する
		clock::time_point next_message_time;
		while (next_message_iter != sequence.end()) {
			auto now_time = clock::now(); // 処理中にも現在時間は変わる
			auto msg_time = start_time + std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>((*next_message_iter)->message.getTimeStamp()));
			auto& msg = (*next_message_iter)->message;

			if (msg_time >= now_time) {
				next_message_time = msg_time;
				break;
			}
			mReceiver.onMidiMessageReceived(msg_time, msg);
			++next_message_iter;
		}

		// 処理すべきメッセージが無くなった場合、停止
		if(next_message_iter == sequence.end()) break;

		// 次のメッセージまで待機
		auto max_sleep_until = clock::now() + max_sleep_duration;
		std::this_thread::sleep_until(std::min(next_message_time, max_sleep_until));
	}
}