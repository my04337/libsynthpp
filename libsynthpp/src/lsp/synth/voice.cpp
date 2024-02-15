/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/synth/voice.hpp>
#include <lsp/synth/sound.hpp>
#include <lsp/synth/synth.hpp>
#include <lsp/synth/state.hpp>
#include <lsp/synth/instruments.hpp>

using namespace lsp::synth;

LuathVoice::LuathVoice(LuathSynth& synth)
	: mSynth(synth)
{
}

LuathVoice::~LuathVoice() = default;

bool LuathVoice::canPlaySound(juce::SynthesiserSound* sound)
{
	return dynamic_cast<LuathSound*>(sound) != nullptr; 
}
bool LuathVoice::isVoiceActive()const
{
	return mEG.isBusy();
}

LuathVoice::Digest LuathVoice::digest()const noexcept
{
	Digest digest;

	if(getCurrentlyPlayingSound() != nullptr) {
		digest.ch = getChannelState().midiChannel;
		digest.freq = mResolvedFreq;
		digest.envelope = mEG.envelope();
		digest.state = mEG.state();
	}

	return digest;
}
void LuathVoice::startNote(int noteNo, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
	auto& midich = getChannelState();

	if(midich.isDrumPart) {
		prepareDrumNote(noteNo, velocity, currentPitchWheelPosition);
	}
	else {
		prepareMelodyNote(noteNo, velocity, currentPitchWheelPosition);
	}
}
void LuathVoice::stopNote(float, bool allowTailOff)
{
	if(allowTailOff) {
		mEG.noteOff();
	}
	else {
		mEG.reset();
	}
}

void LuathVoice::pitchWheelMoved(int newPitchWheelValue)
{
	auto& midich = getChannelState();
	// ピッチベンドからノート番号へ変換
	mPitchBend = midich.pitchBend(midich.synth.systemType());

	// マスターコースチューニング、マスターファインチューニングを反映
	auto masterCourseTuning = static_cast<float>(midich.rpn_getInt7(0, 2).value_or(0)); // 半音単位
	auto masterFineTuning = midich.rpn_getFloat(0, 1, -1.f, +1.f).value_or(0.0); // 最大±1半音

	//最終的なノート番号を算出
	auto resolvedNoteNo 
		= mWaveTableNoteNo
		+ mPitchBend
		+ masterCourseTuning
		+ masterFineTuning;

	// ノート番号から平均律へ変換
	mResolvedFreq = 440 * exp2((resolvedNoteNo - 69.0f) / 12.0f);
}
void LuathVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
	if(getCurrentlyPlayingSound() == nullptr) return;

	lsp_require(outputBuffer.getNumChannels() == 2);

	auto& midich = getChannelState();

	// オシレータからの出力はモノラル
	auto lch = outputBuffer.getWritePointer(0);
	auto rch = outputBuffer.getWritePointer(1);

	for(int i = 0; i < numSamples; ++i) {
		// 基本となる波形を生成 (モノラル)
		auto v = mWG.update(sampleFreq(), mResolvedFreq);
		v = mCutOffFilter.update(v);
		v = mResonanceFilter.update(v);
		v *= mEG.update();
		v *= mVolume;

		// チャネル ボリューム
		v *= midich.ccVolume;

		// エクスプレッション
		v *= midich.ccExpression;

		// パン適用
		float pan = mChannelPan;
		if(mPerVoicePan.has_value()) {
			float vpan = *mPerVoicePan;
			if(vpan < 0.5f) {
				// vpan=0 : 左, vpan=0.5 : 元のpan
				pan = pan * (vpan * 2);
			}
			else {
				// vpan=0.5 : 元のpan, vpan=1.0 : 右
				pan = 1.0f - (1.0f - pan) * ((1.0f - vpan) * 2);
			}
		}
		auto l = v * (1.0f - pan);
		auto r = v * pan;

		// 書き戻し
		lch[i] += l;
		rch[i] += r;
	}

}

void LuathVoice::setCutOff(float freqRate, float cutOffGain)
{
	float freq = mResolvedFreq * freqRate;
	mCutOffFilter.setHighshelfParam(sampleFreq(), freq, 1.f, cutOffGain);
}
void LuathVoice::setResonance(float freqRate, float overtoneGain)
{
	float freq = mResolvedFreq * freqRate;
	mResonanceFilter.setPeakingParam(sampleFreq(), freq, 1.f, overtoneGain);
}
LuathVoice::EnvelopeGenerator& LuathVoice::envolopeGenerator() noexcept
{
	return mEG;
}

const ChannelState& LuathVoice::getChannelState()const noexcept
{
	auto sound = dynamic_cast<LuathSound*>(getCurrentlyPlayingSound().get());
	lsp_check(sound != nullptr);
	auto& synth = sound->synth();
	return synth.getChannelState(sound->midiChannel());

}
void LuathVoice::prepareMelodyNote(int noteNo, float velocity, int currentPitchWheelPosition)
{
	static const std::unordered_map<
		int, std::tuple<
		float, // v: volume(adjuster)
		float, // a: sec
		float, // h: sec
		float, // d: sec
		float, // s: level
		float, // f: Linear : level/sec, Exp : dBFS/sec
		float // r: sec
		>
	> params = {
		{0,   { 1.00f, 0.02f, 0.00f,  3.00f, 0.00f, 0.00f, 1.00f }},
		{1,   { 1.00f, 0.02f, 0.00f,  5.00f, 0.00f, 0.00f, 1.00f }},
		{2,   { 1.00f, 0.02f, 0.00f,  5.00f, 0.00f, 0.00f, 1.00f }},
		{3,   { 1.00f, 0.02f, 0.00f,  5.00f, 0.00f, 0.00f, 1.00f }},
		{4,   { 1.00f, 0.02f, 0.00f,  5.00f, 0.00f, 0.00f, 1.00f }},
		{5,   { 1.00f, 0.02f, 0.00f,  5.00f, 0.00f, 0.00f, 1.00f }},
		{6,   { 1.00f, 0.02f, 0.00f,  5.00f, 0.00f, 0.00f, 1.00f }},
		{7,   { 1.00f, 0.02f, 0.00f,  6.00f, 0.00f, 0.00f, 0.10f }},
		{8,   { 1.00f, 0.02f, 0.00f,  4.00f, 0.00f, 0.00f, 1.50f }},
		{9,   { 1.00f, 0.02f, 0.00f,  4.00f, 0.00f, 0.00f, 2.00f }},
		{10,  { 1.00f, 0.02f, 0.00f,  4.00f, 0.00f, 0.00f, 2.00f }},
		{11,  { 1.00f, 0.02f, 0.00f,  7.00f, 0.00f, 0.00f, 1.00f }},
		{12,  { 1.00f, 0.02f, 0.00f,  3.00f, 0.00f, 0.00f, 2.00f }},
		{13,  { 1.00f, 0.02f, 0.00f,  3.00f, 0.00f, 0.00f, 2.00f }},
		{14,  { 1.00f, 0.02f, 0.00f,  7.00f, 0.00f, 0.00f, 5.00f }},
		{15,  { 1.00f, 0.02f, 0.00f,  3.00f, 0.00f, 0.00f, 2.00f }},
		{16,  { 1.00f, 0.02f, 0.00f,  0.50f, 0.70f, 0.00f, 0.10f }},
		{17,  { 1.00f, 0.02f, 0.00f,  0.50f, 0.70f, 0.00f, 0.10f }},
		{18,  { 1.00f, 0.02f, 0.00f,  0.50f, 0.70f, 0.00f, 0.10f }},
		{19,  { 1.00f, 0.02f, 0.00f,  0.50f, 0.70f, 0.00f, 1.00f }},
		{20,  { 1.00f, 0.02f, 0.00f,  1.00f, 0.50f, 0.00f, 1.00f }},
		{21,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{22,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{23,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{24,  { 1.00f, 0.02f, 0.00f,  7.00f, 0.00f, 0.00f, 0.10f }},
		{25,  { 1.00f, 0.02f, 0.00f,  5.00f, 0.00f, 0.00f, 0.20f }},
		{26,  { 1.00f, 0.02f, 0.00f,  6.00f, 0.00f, 0.00f, 0.20f }},
		{27,  { 1.00f, 0.02f, 0.00f,  5.00f, 0.00f, 0.00f, 0.20f }},
		{28,  { 1.00f, 0.02f, 0.00f,  5.00f, 0.00f, 0.00f, 0.20f }},
		{29,  { 1.00f, 0.02f, 0.00f,  7.00f, 0.08f, 0.00f, 0.10f }},
		{30,  { 1.00f, 0.02f, 0.20f,  7.00f, 0.08f, 0.00f, 0.10f }},
		{31,  { 1.00f, 0.02f, 0.20f, 10.00f, 0.00f, 0.00f, 0.10f }},
		{32,  { 1.00f, 0.02f, 0.00f,  5.00f, 0.08f, 0.00f, 0.10f }},
		{33,  { 1.00f, 0.02f, 0.00f,  5.00f, 0.03f, 0.00f, 0.10f }},
		{34,  { 1.00f, 0.02f, 0.00f,  4.00f, 0.06f, 0.00f, 0.10f }},
		{35,  { 1.00f, 0.02f, 0.00f, 10.00f, 0.00f, 0.00f, 0.10f }},
		{36,  { 1.00f, 0.02f, 0.00f,  3.00f, 0.01f, 0.00f, 0.10f }},
		{37,  { 1.00f, 0.02f, 0.00f,  3.00f, 0.01f, 0.00f, 0.10f }},
		{38,  { 1.00f, 0.02f, 0.00f,  1.50f, 0.07f, 0.00f, 0.10f }},
		{39,  { 1.00f, 0.02f, 0.20f,  5.00f, 0.13f, 0.00f, 0.10f }},
		{40,  { 1.00f, 0.03f, 0.20f,  5.00f, 0.66f, 0.00f, 0.20f }},
		{41,  { 1.00f, 0.03f, 0.35f,  5.00f, 0.66f, 0.00f, 0.20f }},
		{42,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.80f, 0.00f, 0.20f }},
		{43,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.80f, 0.00f, 0.20f }},
		{44,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.90f, 0.00f, 1.00f }},
		{45,  { 1.00f, 0.03f, 0.00f,  0.50f, 0.00f, 0.00f, 0.50f }},
		{46,  { 1.00f, 0.02f, 0.00f,  4.00f, 0.00f, 0.00f, 3.00f }},
		{47,  { 3.50f, 0.02f, 0.00f,  0.30f, 0.00f, 0.00f, 0.30f }},
		{48,  { 1.00f, 0.02f, 0.00f,  0.30f, 0.80f, 0.00f, 0.70f }},
		{49,  { 1.00f, 0.03f, 0.00f,  0.30f, 0.80f, 0.00f, 1.20f }},
		{50,  { 1.00f, 0.03f, 0.00f,  0.30f, 0.80f, 0.00f, 0.70f }},
		{51,  { 1.00f, 0.03f, 0.00f,  0.30f, 0.80f, 0.00f, 1.20f }},
		{52,  { 1.00f, 0.03f, 0.00f,  0.30f, 0.80f, 0.00f, 0.70f }},
		{53,  { 1.00f, 0.03f, 0.00f,  0.30f, 0.80f, 0.00f, 0.70f }},
		{54,  { 1.00f, 0.03f, 0.00f,  0.50f, 0.80f, 0.00f, 0.70f }},
		{55,  { 2.00f, 0.02f, 0.10f,  0.60f, 0.00f, 0.00f, 0.40f }},
		{56,  { 1.00f, 0.03f, 0.00f,  0.50f, 0.60f, 0.00f, 0.10f }},
		{57,  { 1.00f, 0.03f, 0.00f,  4.00f, 0.60f, 0.00f, 0.10f }},
		{58,  { 1.00f, 0.03f, 0.00f,  2.00f, 0.60f, 0.00f, 0.10f }},
		{59,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.60f, 0.00f, 0.10f }},
		{60,  { 1.00f, 0.03f, 0.00f,  2.00f, 0.60f, 0.00f, 0.10f }},
		{61,  { 1.00f, 0.03f, 0.00f,  1.50f, 0.50f, 0.00f, 0.10f }},
		{62,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.50f, 0.00f, 0.10f }},
		{63,  { 1.00f, 0.03f, 0.00f,  2.00f, 0.50f, 0.00f, 0.10f }},
		{64,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{65,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{66,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{67,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{68,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{69,  { 1.00f, 0.03f, 0.00f,  0.50f, 0.80f, 0.00f, 0.10f }},
		{70,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{71,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{72,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{73,  { 1.00f, 0.02f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{74,  { 1.00f, 0.02f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{75,  { 1.00f, 0.02f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{76,  { 1.00f, 0.10f, 0.00f,  0.50f, 0.80f, 0.00f, 0.10f }},
		{77,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.80f, 0.00f, 0.10f }},
		{78,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.80f, 0.00f, 0.30f }},
		{79,  { 1.00f, 0.03f, 0.00f,  1.00f, 0.80f, 0.00f, 0.10f }},
		{80,  { 1.00f, 0.02f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{81,  { 1.00f, 0.02f, 0.00f,  1.00f, 0.80f, 0.00f, 0.10f }},
		{82,  { 1.00f, 0.02f, 0.00f,  1.00f, 0.60f, 0.00f, 0.10f }},
		{83,  { 1.00f, 0.03f, 0.00f,  4.00f, 0.25f, 0.00f, 0.10f }},
		{84,  { 1.00f, 0.03f, 0.03f,  4.00f, 0.25f, 0.00f, 0.10f }},
		{85,  { 1.00f, 0.03f, 0.00f,  4.00f, 0.56f, 0.00f, 0.20f }},
		{86,  { 1.00f, 0.03f, 0.15f,  4.00f, 0.31f, 0.00f, 0.30f }},
		{87,  { 1.00f, 0.03f, 0.00f,  4.50f, 0.31f, 0.00f, 0.10f }},
		{88,  { 1.00f, 0.03f, 0.00f,  3.00f, 0.56f, 0.00f, 3.00f }},
		{89,  { 1.00f, 0.03f, 0.00f,  3.00f, 0.90f, 0.00f, 1.00f }},
		{90,  { 1.00f, 0.03f, 0.00f,  3.00f, 0.40f, 0.00f, 0.80f }},
		{91,  { 1.00f, 0.03f, 0.00f,  3.00f, 0.90f, 0.00f, 3.00f }},
		{92,  { 1.00f, 0.03f, 0.00f,  3.00f, 0.90f, 0.00f, 2.50f }},
		{93,  { 1.00f, 0.30f, 0.00f,  8.00f, 0.00f, 0.00f, 2.00f }},
		{94,  { 1.00f, 0.03f, 0.15f,  1.00f, 0.90f, 0.00f, 1.00f }},
		{95,  { 1.00f, 0.30f, 0.00f,  5.00f, 0.50f, 0.00f, 2.00f }},
		{96,  { 1.00f, 0.03f, 0.00f,  4.00f, 0.50f, 0.00f, 2.00f }},
		{97,  { 1.00f, 0.03f, 0.10f,  3.00f, 0.70f, 0.00f, 3.00f }},
		{98,  { 1.00f, 0.03f, 0.00f,  4.00f, 0.00f, 0.00f, 3.00f }},
		{99,  { 1.00f, 0.03f, 0.00f,  4.00f, 0.10f, 0.00f, 2.00f }},
		{100, { 1.00f, 0.03f, 0.10f,  5.00f, 0.00f, 0.00f, 3.00f }},
		{101, { 1.00f, 1.00f, 0.00f,  1.00f, 0.90f, 0.00f, 2.00f }},
		{102, { 1.00f, 0.03f, 0.00f,  1.00f, 0.90f, 0.00f, 2.00f }},
		{103, { 1.00f, 0.03f, 0.00f,  6.00f, 0.04f, 0.00f, 2.00f }},
		{104, { 1.00f, 0.03f, 0.03f,  9.00f, 0.00f, 0.00f, 1.00f }},
		{105, { 1.00f, 0.03f, 0.00f,  6.00f, 0.00f, 0.00f, 1.00f }},
		{106, { 1.00f, 0.03f, 0.00f,  3.00f, 0.00f, 0.00f, 1.00f }},
		{107, { 1.00f, 0.03f, 0.00f,  3.00f, 0.00f, 0.00f, 2.00f }},
		{108, { 1.00f, 0.03f, 0.00f,  1.50f, 0.00f, 0.00f, 1.00f }},
		{109, { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{110, { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{111, { 1.00f, 0.03f, 0.00f,  1.00f, 0.70f, 0.00f, 0.10f }},
		{112, { 1.00f, 0.03f, 0.00f,  3.00f, 0.00f, 0.00f, 1.00f }},
		{113, { 1.00f, 0.03f, 0.00f,  0.40f, 0.00f, 0.00f, 0.30f }},
		{114, { 1.00f, 0.03f, 0.00f,  3.00f, 0.00f, 0.00f, 2.00f }},
		{115, { 1.00f, 0.03f, 0.00f,  0.50f, 0.00f, 0.00f, 0.50f }},
		{116, { 1.00f, 0.03f, 0.00f,  3.00f, 0.00f, 0.00f, 3.00f }},
		{117, { 1.00f, 0.03f, 0.00f,  0.80f, 0.00f, 0.00f, 0.80f }},
		{118, { 1.00f, 0.03f, 0.00f,  0.80f, 0.00f, 0.00f, 0.80f }},
		{119, { 1.00f, 2.00f, 0.00f,  0.10f, 0.00f, 0.00f, 0.10f }},
		{120, { 1.00f, 0.03f, 0.00f,  0.50f, 0.00f, 0.00f, 0.50f }},
		{121, { 1.00f, 0.03f, 0.00f,  0.50f, 0.00f, 0.00f, 0.40f }},
		{122, { 2.00f, 0.50f, 1.50f,  3.00f, 0.00f, 0.00f, 1.50f }},
		{123, { 2.00f, 0.50f, 0.80f,  4.00f, 0.00f, 0.00f, 0.50f }},
		{124, { 2.00f, 0.03f, 0.00f,  1.00f, 0.80f, 0.00f, 0.10f }},
		{125, { 2.00f, 3.50f, 0.00f,  1.00f, 0.80f, 0.00f, 0.70f }},
		{126, { 2.00f, 2.00f, 0.00f,  1.00f, 0.80f, 0.00f, 0.80f }},
		{127, { 2.50f, 0.03f, 0.00f,  1.50f, 0.00f, 0.00f, 1.50f }},
	};

	auto& midich = getChannelState();


	float v = 1.f; // volume(adjuster)
	float a = 0.f; // sec
	float h = 0.f; // sec
	float d = 0.f; // sec
	float s = 1.f; // level
	float f = 0.f; // Linear : level/sec, Exp : dBFS/sec
	float r = 0.f; // sec
	

	// 主要パラメータのロード
	if(auto found = params.find(midich.progId); found != params.end()) {
		std::tie(v, a, h, d, s, f, r) = found->second;
	}

	// 楽器毎の調整
	bool isDrumLikeInstrument = false;
	float noteNoAdjuster = 0;
	switch(midich.progId)
	{
	case 47:
	case 121:
	case 122:
	case 125:
	case 126:
	case 127:
		// リズム系の楽器はドラム系のパラメータを用いるようにする ※ただしドラムパートほど強くパラメータは変更しない
		isDrumLikeInstrument = true;
		noteNoAdjuster -= 12; // 1オクターブ下げる
		break;
	}


	// 音量の反映
	// - エンベロープ
	if(mSynth.systemType().isGS() || mSynth.systemType().isXG()) {
		a *= powf(10.0f, (midich.ccAttackTime - 0.5f) * 4.556f);
		d *= powf(10.0f, (midich.ccDecayTime - 0.5f) * 4.556f);
		r *= powf(10.0f, (midich.ccReleaseTime - 0.5f) * 4.556f);
	}
	float cutoff_level = 0.01f/*≒-66.4dB*/; // ほぼ無音を長々再生するのを防ぐため、ほぼ聞き取れないレベルまで落ちたらカットオフする
	mEG.setMelodyEnvelope(mSynth.sampleFreq(), dsp::EnvelopeGenerator<float>::Curve(3.0f), a, h, d, s, f, r, cutoff_level);
	mEG.noteOn();

	// - ボリューム ※ベロシティを振幅に変換した物
	// TODO sustain_levelで除算しているのは旧LibSynth++からの移植コード。 補正が不要になったら削除すること
	mVolume = powf(10.f, -20.f * (1.f - velocity) / 20.f) * v / ((s > 0.8f && s != 0.f) ? s : 0.8f);


	// 音色の反映
	// - 波形テーブル
	if(isDrumLikeInstrument) {
		mWG = Instruments::createDrumNoiseGenerator();
	}
	else {
		// 素の正弦波は結構どぎつい倍音を持つため、折り返しノイズ等を避けるために大まかなノート番号によって倍音成分を変化させる
		// ※正弦波の倍音は1～50まで用意してあるため、良い感じにマッピングする
		static constexpr float min_note_no = 0;  // C-1 : 8.2[Hz]
		static constexpr float max_note_no = 108; // C8 : 4186.0[Hz]
		static constexpr int min_dim = 5;
		static constexpr int max_dim = 30;

		auto waveTableId = std::clamp(
			static_cast<int>(min_dim + (1.f - (noteNo - min_note_no) / (max_note_no - min_note_no)) * (max_dim - min_dim)),
			min_dim,
			max_dim
		);
		mWG = Instruments::createSquareGenerator(waveTableId);
	}

	// 音程反映
	// - 指示されたノート番号 + 学期毎の補正
	mWaveTableNoteNo = noteNo + noteNoAdjuster;

	// - ピッチベンド
	pitchWheelMoved(currentPitchWheelPosition); // 最終的な周波数はこの先で反映する

	// - カットオフおよび共振パラメータ指定
	float cutOffFreqRate = 2.f;
	float overtuneGain = 0.f; // dB
	if(mSynth.systemType().isGS() || mSynth.systemType().isXG()) {
		cutOffFreqRate = midich.nrpn_getFloat(1, 32, 0.f, 1.f).value_or(0.5f) * 2.f + 1.f;
		overtuneGain = midich.nrpn_getFloat(1, 33, -1.f, +1.f).value_or(0) * 5.f;
	}
	setResonance(cutOffFreqRate, overtuneGain);


	// その他反映
	// - パン(位相)
	mChannelPan = midich.ccPanpot;
	mPerVoicePan = std::nullopt;
}
void LuathVoice::prepareDrumNote(int noteNo, float velocity, int currentPitchWheelPosition)
{
	static const std::unordered_map<
		int, std::tuple<
		int,// pitch : NoteNo
		float, // v: volume(adjuster)
		float, // a: sec
		float, // h: sec
		float, // d: sec
		float  // pan : 0～1
		>
	> params = {
		{ 24, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.35f}}, //	
		{ 25, { 57, 1.50f, 0.05f, 0.00f, 1.00f, 0.50f}}, //
		{ 26, { 81, 2.00f, 0.00f, 0.00f, 0.20f, 0.50f}}, //
		{ 27, { 64, 2.00f, 0.00f, 0.00f, 0.20f, 0.50f}}, // High Q
		{ 28, { 69, 2.00f, 0.00f, 0.00f, 0.30f, 0.42f}}, // Slap
		{ 29, { 69, 1.80f, 0.00f, 0.00f, 0.30f, 0.50f}}, // Scratch Push
		{ 30, { 66, 1.80f, 0.00f, 0.00f, 0.40f, 0.50f}}, // Scratch Pull
		{ 31, { 69, 3.00f, 0.00f, 0.00f, 0.20f, 0.50f}}, // Sticks
		{ 32, { 69, 2.00f, 0.00f, 0.00f, 0.10f, 0.50f}}, // Square Click
		{ 33, { 66, 2.00f, 0.00f, 0.00f, 0.10f, 0.50f}}, // Metronome Click
		{ 34, { 78, 2.00f, 0.00f, 0.00f, 0.30f, 0.50f}}, // Metronome Bell
		{ 35, { 32, 4.59f, 0.00f, 0.00f, 0.20f, 0.50f}}, // Acoustic Bass Drum
		{ 36, { 32, 4.50f, 0.00f, 0.00f, 0.25f, 0.50f}}, // Bass Drum 1
		{ 37, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.50f}}, // Side Stick
		{ 38, { 44, 4.00f, 0.00f, 0.00f, 0.40f, 0.50f}}, // Snare 1
		{ 39, { 57, 2.00f, 0.00f, 0.05f, 0.40f, 0.42f}}, // Hand Clap
		{ 40, { 64, 3.00f, 0.00f, 0.00f, 0.40f, 0.50f}}, // Snare 2
		{ 41, { 44, 3.00f, 0.00f, 0.00f, 0.50f, 0.27f}}, // Low Floor Tom
		{ 42, { 64, 1.60f, 0.00f, 0.05f, 0.10f, 0.66f}}, // Closed Hi-hat
		{ 43, { 46, 3.00f, 0.00f, 0.00f, 0.50f, 0.36f}}, // High Floor Tom
		{ 44, { 66, 1.60f, 0.00f, 0.05f, 0.10f, 0.66f}}, // Pedal Hi-hat
		{ 45, { 48, 3.00f, 0.00f, 0.00f, 0.50f, 0.45f}}, // Low Tom
		{ 46, { 70, 1.60f, 0.00f, 0.10f, 0.75f, 0.66f}}, // Open Hi-hat
		{ 47, { 50, 3.00f, 0.00f, 0.00f, 0.40f, 0.55f}}, // Low-Mid Tom
		{ 48, { 53, 3.00f, 0.00f, 0.00f, 0.40f, 0.64f}}, // High-Mid Tom
		{ 49, { 68, 2.00f, 0.00f, 0.00f, 2.00f, 0.66f}}, // Crash Cymbal 1
		{ 50, { 56, 3.50f, 0.00f, 0.00f, 0.40f, 0.73f}}, // High Tom
		{ 51, { 66, 2.50f, 0.00f, 0.00f, 1.50f, 0.35f}}, // Ride Cymbal 1
		{ 52, { 62, 2.00f, 0.00f, 0.00f, 1.00f, 0.35f}}, // Chinese Cymbal
		{ 53, { 80, 2.00f, 0.00f, 0.00f, 1.00f, 0.35f}}, // Ride Bell
		{ 54, { 100,2.00f, 0.00f, 0.00f, 0.20f, 0.58f}}, // Tambourine
		{ 55, { 70, 1.60f, 0.00f, 0.00f, 1.50f, 0.42f}}, // Splash Cymbal
		{ 56, { 60, 3.00f, 0.00f, 0.00f, 0.40f, 0.66f}}, // Cowbell
		{ 57, { 72, 2.00f, 0.00f, 0.00f, 1.50f, 0.35f}}, // Crash Cymbal 2
		{ 58, { 60, 2.00f, 0.00f, 0.00f, 1.50f, 0.22f}}, // Vibra-slap
		{ 59, { 76, 1.20f, 0.00f, 0.00f, 1.50f, 0.35f}}, // Ride Cymbal 2
		{ 60, { 70, 2.00f, 0.00f, 0.00f, 0.20f, 0.77f}}, // High Bongo
		{ 61, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.77f}}, // Low Bongo
		{ 62, { 70, 3.00f, 0.00f, 0.00f, 0.10f, 0.30f}}, // Mute Hi Conga
		{ 63, { 67, 2.00f, 0.00f, 0.00f, 0.30f, 0.30f}}, // Open Hi Conga
		{ 64, { 58, 2.00f, 0.00f, 0.00f, 0.35f, 0.35f}}, // Low Conga
		{ 65, { 70, 2.00f, 0.00f, 0.00f, 0.40f, 0.66f}}, // High Timbale
		{ 66, { 64, 2.00f, 0.00f, 0.00f, 0.40f, 0.66f}}, // Low Timbale
		{ 67, { 70, 2.00f, 0.00f, 0.00f, 0.20f, 0.22f}}, // High Agogo
		{ 68, { 64, 2.00f, 0.00f, 0.00f, 0.20f, 0.22f}}, // Low Agogo
		{ 69, { 70, 2.50f, 0.00f, 0.00f, 0.15f, 0.22f}}, // Cabasa
		{ 70, { 66, 2.00f, 0.00f, 0.00f, 0.20f, 0.19f}}, // Maracas
		{ 71, { 71, 2.00f, 0.00f, 0.10f, 0.10f, 0.77f}}, // Short Whistle
		{ 72, { 69, 2.00f, 0.00f, 0.30f, 0.10f, 0.77f}}, // Long Whistle
		{ 73, { 70, 2.50f, 0.00f, 0.00f, 0.15f, 0.73f}}, // Short Guiro
		{ 74, { 68, 1.50f, 0.00f, 0.30f, 0.10f, 0.73f}}, // Long Guiro
		{ 75, { 76, 2.00f, 0.00f, 0.00f, 0.20f, 0.66f}}, // Claves
		{ 76, { 70, 2.00f, 0.00f, 0.00f, 0.30f, 0.77f}}, // Hi Wood Block
		{ 77, { 64, 2.00f, 0.00f, 0.00f, 0.30f, 0.77f}}, // Low Wood Block
		{ 78, { 70, 2.00f, 0.00f, 0.00f, 0.40f, 0.35f}}, // Mute Cuica
		{ 79, { 64, 2.00f, 0.00f, 0.00f, 0.40f, 0.35f}}, // Open Cuica
		{ 80, { 86, 2.00f, 0.00f, 0.00f, 0.35f, 0.19f}}, // Mute Triangle
		{ 81, { 86, 0.70f, 0.00f, 0.10f, 0.20f, 0.19f}}, // Open Triangle
	};

	auto& midich = getChannelState();

	uint8_t pitch = 69;
	float v = 1.f; // volume(adjuster)
	float a = 0.f; // sec
	float h = 0.f; // sec
	float d = 0.f; // sec
	float pan = 0.5;

	// 定義済の主要パラメータのロード
	if(auto found = params.find(noteNo); found != params.end()) {
		std::tie(pitch, v, a, h, d, pan) = found->second;
	}

	// 音量の反映
	// - エンベロープ
	float cutoff_level = 0.01f/*≒-66.4dB*/; // ほぼ無音を長々再生するのを防ぐため、ほぼ聞き取れないレベルまで落ちたらカットオフする
	mEG.setDrumEnvelope(mSynth.sampleFreq(), dsp::EnvelopeGenerator<float>::Curve(3.0f), a, h, d, cutoff_level);
	mEG.noteOn();

	// - ボリューム ※ベロシティを振幅に変換した物
	mVolume = powf(10.f, -20.f * (1.f - velocity) / 20.f) * v;


	// 音色の反映
	// - 波形テーブル
	mWG = Instruments::createDrumNoiseGenerator();

	// 音程反映
	// - ベース : ドラムの音色毎の基底の音程
	mWaveTableNoteNo = pitch;
	
	// - NRPN : ドラムの音程微調整
	mWaveTableNoteNo += static_cast<float>(midich.nrpn_getInt7(24, noteNo).value_or(0));

	// - ピッチベンド
	pitchWheelMoved(currentPitchWheelPosition); // 最終的な周波数はこの先で反映する

	// - カットオフおよび共振パラメータ指定
	float cutOffFreqRate = 2.f;
	float overtuneGain = 0.f; // dB
	if(mSynth.systemType().isGS() || mSynth.systemType().isXG()) {
		cutOffFreqRate = midich.nrpn_getFloat(1, 32, 0.f, 1.f).value_or(0.5f) * 2.f + 1.f;
		overtuneGain = midich.nrpn_getFloat(1, 33, -1.f, +1.f).value_or(0) * 5.f;
	}
	setResonance(cutOffFreqRate, overtuneGain);


	// その他反映
	// - パン(位相)
	mChannelPan = midich.ccPanpot;
	if(auto panFromRPN = midich.nrpn_getInt7(28, noteNo)) {
		// NRPN : パンが指定されている場合、オーバーライドする
		mPerVoicePan = panFromRPN.value_or(rand() % 128 - 64) / 127.f + 0.5f;
	}
	else {
		mPerVoicePan = std::nullopt;
	}
}