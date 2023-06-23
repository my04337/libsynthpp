#pragma once

#include <lsp/base/base.hpp>
#include <lsp/midi/synth/midi_channel.hpp>
#include <lsp/midi/synth/wave_table.hpp>

#include <lsp/midi/message_receiver.hpp>

#include <array>
#include <shared_mutex>

namespace lsp::midi::synth
{

class Synthesizer
	: public midi::MessageReceiver
{
public:
	using RenderingCallback = std::function<void(Signal<float>&& sig)>;
	static constexpr uint8_t MAX_CHANNELS = 16;
	struct Statistics {
		uint64_t created_samples = 0;
		uint64_t failed_samples = 0;

		clock::duration cycle_time;
		clock::duration rendering_time;
		float rendering_load_average()const noexcept {
			if (cycle_time.count() > 0) {
				return (float)rendering_time.count() / (float)cycle_time.count();
			} else {
				return 0;
			}
		}
	};
	struct Digest {
		midi::SystemType systemType;

		std::vector<MidiChannel::Digest> channels;
	};

public:
	Synthesizer(uint32_t sampleFreq, midi::SystemType defaultSystemType = midi::SystemType::GS);
	~Synthesizer();

	void dispose();

	// MIDIメッセージを受信した際にコールバックされます。
	virtual void onMidiMessageReceived(clock::time_point received_time, const std::shared_ptr<const midi::Message>& msg)override;
	
	// 音声が生成された際のコールバック関数を設定します
	void setRenderingCallback(RenderingCallback cb);

	// 統計情報を取得します
	Statistics statistics()const;
	// 現在の内部状態のダイジェストを取得します
	Digest digest()const;

protected:
	void playingThreadMain();
	void dispatchMessage(const std::shared_ptr<const midi::Message>& msg);
	void reset(midi::SystemType type);

	// システムエクスクルーシブ
	void sysExMessage(const uint8_t* data, size_t len);

	// MIDIメッセージを元に演奏した結果を返します
	Signal<float> generate(size_t len);

private:
	mutable std::shared_mutex mMutex;
	std::pmr::synchronized_pool_resource mMem;
	std::deque<std::pair<clock::time_point, std::shared_ptr<const midi::Message>>> mMessageQueue;

	Statistics mStatistics;
	std::atomic<Statistics> mThreadSafeStatistics;

	RenderingCallback mRenderingCallback;
		
	// all channel parameters
	const uint32_t mSampleFreq;
	midi::SystemType mSystemType;
	WaveTable mWaveTable;

	// midi channel parameters
	std::vector<MidiChannel> mMidiChannels;

	// 演奏スレッド
	std::thread mPlayingThread;
	std::atomic_bool mPlayingThreadAborted;
};

}