// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/system_type.hpp>
#include <lsp/synth/state.hpp>
#include <lsp/synth/voice.hpp>

#include <array>
#include <shared_mutex>

namespace lsp::synth
{
class LuathVoice;

class LuathSynth final
	: public juce::Synthesiser
{
	friend class LuathVoice;
	using SUPER = juce::Synthesiser;

public:
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

		std::vector<ChannelState::Digest> channels;
		std::vector<LuathVoice::Digest> voices;
	};

public:
	LuathSynth();
	~LuathSynth();

	void dispose();

	// 信号を生成します
	void renderNextBlock(juce::AudioBuffer<float>& outputAudio, const juce::MidiBuffer& inputMidi, int startSample, int numSamples);

	// サンプリング周波数を指定します
	void setCurrentPlaybackSampleRate(double sampleRate)override;

	// サンプリング周波数を返します
	float sampleFreq()const noexcept { return mSampleFreq; }

	// 統計情報を取得します
	Statistics statistics()const;
	// 現在の内部状態のダイジェストを取得します
	Digest digest()const;
	// MIDIシステムタイプ(リセット種別)を取得します
	midi::SystemType systemType()const noexcept;

protected:
	void reset(midi::SystemType defaultSystemType = midi::SystemType::GS());

	ChannelState& getChannelState(int ch)noexcept;
	const ChannelState& getChannelState(int ch)const noexcept;

public: // implementation of juce::Synthesizer +α
	template<class F, class Voice> requires std::invocable<F, Voice&>
	void forEachVoice(int channel, F f) {
		for(auto& voice : voices) {
			if(voice->isPlayingChannel(channel)) {
				if(auto v = dynamic_cast<Voice*>(voice)) {
					f(*v);
				}
			}
		}
	}

	void handlePitchWheel(int channel, int value)override;
	void handleController(int channel, int ctrlNo, int value)override;
	void handleProgramChange(int channel, int progId)override;

	void sysExMessage(const uint8_t* data, size_t len);

protected: // implementation of juce::Synthesizer
	void handleMidiEvent(const juce::MidiMessage&) override;

private:
	Statistics mStatistics;
	std::atomic<Statistics> mThreadSafeStatistics;

	// 最終段エフェクタ
	dsp::BiquadraticFilter<float> mFinalLpfL;
	dsp::BiquadraticFilter<float> mFinalLpfR;
	float mMasterVolume;

	// all channel params
	float mSampleFreq;
	midi::SystemType mSystemType;

	// per channel params
	std::vector<ChannelState> mChannelState;
};

}