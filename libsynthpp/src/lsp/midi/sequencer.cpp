// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#include <lsp/midi/sequencer.hpp>
#include <lsp/midi/system_type.hpp>
#include <lsp/util/thread_priority.hpp>

using namespace lsp;
using namespace lsp::midi;


Sequencer::Sequencer(juce::MidiInputCallback& receiver)
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
		// 停止要求があった場合、状態を完全に破棄するためリセットを送出する
		if(stopToken.stop_requested()) {
			std::vector<uint8_t> reset {0x7E, 0x7F, 0x09, 0x01}; // GM1 Reset
			mReceiver.handleIncomingMidiMessage(nullptr, juce::MidiMessage::createSysExMessage(reset.data(), static_cast<int>(reset.size())));
			break;
		}

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
			mReceiver.handleIncomingMidiMessage(nullptr, msg);
			++next_message_iter;
		}

		// 処理すべきメッセージが無くなった場合、停止
		if(next_message_iter == sequence.end()) break;

		// 次のメッセージまで待機
		auto max_sleep_until = clock::now() + max_sleep_duration;
		std::this_thread::sleep_until(std::min(next_message_time, max_sleep_until));
	}
}