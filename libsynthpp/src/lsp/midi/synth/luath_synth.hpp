/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/system_type.hpp>
#include <lsp/midi/message_receiver.hpp>
#include <lsp/midi/synth/midi_channel.hpp>
#include <lsp/midi/synth/wave_table.hpp>

#include <array>
#include <shared_mutex>

namespace lsp::midi::synth
{

class LuathSynth
	: public midi::MessageReceiver
	, public juce::Synthesiser
{
public:
	static constexpr uint8_t MAX_CHANNELS = 16;
	struct Statistics {
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
		float masterVolume;

		std::vector<MidiChannel::Digest> channels;
	};

public:
	LuathSynth(float sampleFreq, midi::SystemType defaultSystemType = midi::SystemType::GS());
	~LuathSynth();

	void dispose();

	// MIDIメッセージを受信した際にコールバックされます。
	virtual void onMidiMessageReceived(clock::time_point received_time, const juce::MidiMessage& msg)override;

	// サンプリング周波数を返します
	float sampleFreq()const noexcept { return mSampleFreq; }

	// 統計情報を取得します
	Statistics statistics()const;
	// 現在の内部状態のダイジェストを取得します
	Digest digest()const;
	// MIDIメッセージを元に演奏した結果を返します
	Signal<float> generate(size_t len);

	// 波形テーブル(プリセット)を返します
	const WaveTable& presetWaveTable()const noexcept { return mPresetWaveTable; }
	// 波形テーブル(正弦波)を返します
	const WaveTable& squareWaveTable()const noexcept { return mSquareWaveTable; }

protected:
	void reset(midi::SystemType type);

public: // implementation of juce::Synthesizer +α
	

	void noteOn(int channel, int noteNo, float velocity)override;
	void noteOff(int channel, int noteno, float velocity, bool allowTailOff)override;
	void allNotesOff(int channel, bool allowTailOff)override;
	void handlePitchWheel(int channel, int value)override;
	void handleController(int channel, int ctrlNo, int value)override;
	void handleProgramChange(int channel, int progId)override;

	void sysExMessage(const uint8_t* data, size_t len);

protected: // implementation of juce::Synthesizer
	void handleMidiEvent(const juce::MidiMessage&) override;

private:
	mutable std::shared_mutex mMutex;
	std::deque<std::pair<clock::time_point, juce::MidiMessage>> mMessageQueue;

	Statistics mStatistics;
	std::atomic<Statistics> mThreadSafeStatistics;

	// 最終段エフェクタ
	effector::BiquadraticFilter<float> mFinalLpfL;
	effector::BiquadraticFilter<float> mFinalLpfR;
	float mMasterVolume;

	// Wavetables
	WaveTable mPresetWaveTable;
	WaveTable mSquareWaveTable;

	// all channel parameters
	const float mSampleFreq;
	midi::SystemType mSystemType;

	// midi channel parameters
	std::vector<MidiChannel> mMidiChannels;
};

}