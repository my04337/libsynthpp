/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>

#include <lsp/dsp/envelope_generator.hpp>
#include <lsp/dsp/biquadratic_filter.hpp>
#include <lsp/dsp/wave_table_generator.hpp>

namespace lsp::synth
{
class LuathSynth;

// Luath用サウンド 
// MEMO juce::SynthesizerVoiceは何らかのjuce::SynthesizerSoundに属している必要があるため、最低限の物を用意した。
class LuathSound final
	: public juce::SynthesiserSound
{
public:
	LuathSound(LuathSynth& synth) : mSynth(synth) {}

	bool appliesToNote(int midiNoteNumber)override { return false; }
	bool appliesToChannel(int midiChannel)override { return false; }

	LuathSynth& synth()const noexcept { return mSynth; }

private:
	LuathSynth& mSynth;
};

// ボイス識別番号
struct _voice_id_tag {};
using VoiceId = issuable_id_base_t<_voice_id_tag>;


// ボイス(あるチャネルの1音) - Luath用波形テーブル音源
class LuathVoice final
	: public juce::SynthesiserVoice
	, non_copy_move
{
public:
	using EnvelopeGenerator = dsp::EnvelopeGenerator<float>;
	using EnvelopeState = dsp::EnvelopeState;
	using BiquadraticFilter = dsp::BiquadraticFilter<float>;
	using WaveTableGenerator = dsp::WaveTableGenerator<float>;

	struct Digest {
		float freq = 0; // 基本周波数
		float envelope = 0; // エンベロープジェネレータ出力
		dsp::EnvelopeState state = dsp::EnvelopeState::Free; // エンベロープジェネレータ ステート
	};

public:
	LuathVoice(const WaveTableGenerator& wg, float noteNo, float pitchBend, float volume, bool hold);
	virtual ~LuathVoice();

	// --- implmentation of uce::SynthesiserVoice --- 
	bool canPlaySound(juce::SynthesiserSound*)override;
	void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) {} // TODO
	void stopNote(float velocity, bool allowTailOff) {} // TODO
	void pitchWheelMoved(int newPitchWheelValue) {} // TODO
	void controllerMoved(int controllerNumber, int newControllerValue) {} // TODO
	void renderNextBlock(juce::AudioBuffer< float >& outputBuffer, int startSample, int numSamples) {} // TODO
	// ---

	float update()
	{
		auto v = mWG.update(sampleFreq(), mCalculatedFreq);
		v = mCutOffFilter.update(v);
		v = mResonanceFilter.update(v);
		v *= mEG.update();
		v *= mVolume;
		return v;
	}

	Digest digest()const noexcept;
	float sampleFreq()const noexcept { return static_cast<float>(getSampleRate()); }

	float noteNo()const noexcept;
	void noteOff(bool allowTailOff = true)noexcept;

	std::optional<float> pan()const noexcept;
	void setPan(float pan)noexcept;

	void setPitchBend(float pitchBend)noexcept;
	EnvelopeGenerator& envolopeGenerator() noexcept;

	void setCutOff(float freqRate, float cutOffGain);
	void setResonance(float freqRate, float overtoneGain);


protected:
	void updateFreq()noexcept;

protected:
	EnvelopeGenerator mEG;
	BiquadraticFilter mCutOffFilter;
	BiquadraticFilter mResonanceFilter;
	WaveTableGenerator mWG;
	float mNoteNo;
	bool mPendingNoteOff = false;
	float mPitchBend;
	float mCalculatedFreq = 0;
	float mVolume;
	std::optional<float> mPan; // ドラムなど、ボイス毎にパンが指定される場合のヒント
};

}