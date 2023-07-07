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
struct ChannelState;


// ボイス識別番号
struct _voice_id_tag {};
using VoiceId = issuable_id_base_t<_voice_id_tag>;


// ボイス(あるチャネルの1音) - Luath用波形テーブル音源
class LuathVoice final
	: public juce::SynthesiserVoice
	, non_copy_move
{
	using SUPER = juce::SynthesiserVoice;

public:
	using EnvelopeGenerator = dsp::EnvelopeGenerator<float>;
	using EnvelopeState = dsp::EnvelopeState;
	using BiquadraticFilter = dsp::BiquadraticFilter<float>;
	using WaveTableGenerator = dsp::WaveTableGenerator<float>;

	struct Digest {
		int ch = 1; // チャネル [1～16]
		float freq = 0; // 基本周波数
		float envelope = 0; // エンベロープジェネレータ出力
		dsp::EnvelopeState state = dsp::EnvelopeState::Free; // エンベロープジェネレータ ステート
	};

public:
	LuathVoice(LuathSynth& synth);
	virtual ~LuathVoice();

	// --- implmentation of uce::SynthesiserVoice --- 
	bool canPlaySound(juce::SynthesiserSound*)override;
	bool isVoiceActive()const override;
	void startNote(int noteNo, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition);
	void stopNote(float velocity, bool allowTailOff);
	void pitchWheelMoved(int newPitchWheelValue);
	void controllerMoved(int controllerNumber, int newControllerValue) {} // TODO
	void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);
	// ---

	Digest digest()const noexcept;
	float sampleFreq()const noexcept { return static_cast<float>(getSampleRate()); }

	EnvelopeGenerator& envolopeGenerator() noexcept;

	void setCutOff(float freqRate, float cutOffGain);
	void setResonance(float freqRate, float overtoneGain);


private:
	const ChannelState& getChannelState()const noexcept;
	void prepareMelodyNote(int noteNo, float velocity, int currentPitchWheelPosition);
	void prepareDrumNote(int noteNo, float velocity, int currentPitchWheelPosition);

private:
	LuathSynth& mSynth;
	EnvelopeGenerator mEG;
	BiquadraticFilter mCutOffFilter;
	BiquadraticFilter mResonanceFilter;
	WaveTableGenerator mWG;
	float mWaveTableNoteNo = 0; // [NoteNo]
	float mPitchBend = 0;       // [NoteNo]
	float mResolvedFreq = 0;    // [Hz]
	float mVolume = 0;
	float mChannelPan = 0.5f; // チャネル全体のパン
	std::optional<float> mPerVoicePan; // ドラムなど、ボイス毎にパンが指定される場合のヒント
};

}