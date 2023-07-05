/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/synth/voice.hpp>

using namespace lsp::synth;

LuathVoice::LuathVoice(const WaveTableGenerator& wg, float noteNo, float pitchBend, float volume, bool hold)
	: mWG(wg)
	, mNoteNo(noteNo)
	, mPitchBend(pitchBend)
	, mVolume(volume)
{
	updateFreq();
}

LuathVoice::~LuathVoice() = default;

bool LuathVoice::canPlaySound(juce::SynthesiserSound* sound)
{
	return dynamic_cast<LuathSound*>(sound) != nullptr; 
} 

LuathVoice::Digest LuathVoice::digest()const noexcept
{
	Digest digest;

	digest.freq = mCalculatedFreq;
	digest.envelope = mEG.envelope();
	digest.state = mEG.state();

	return digest;
}
float LuathVoice::noteNo()const noexcept
{
	return mNoteNo;
}

void LuathVoice::noteOff(bool allowTailOff)noexcept
{
	if(allowTailOff) {
		if(isSustainPedalDown()) {
			mPendingNoteOff = true;
		}
		else {
			mPendingNoteOff = false;
			mEG.noteOff();
		}
	}
	else {
		mPendingNoteOff = false;
		mEG.reset();
	}
}

std::optional<float> LuathVoice::pan()const noexcept
{
	return mPan;
}
void LuathVoice::setPan(float pan)noexcept
{
	mPan = pan;
}
void LuathVoice::setPitchBend(float pitchBend)noexcept
{
	mPitchBend = pitchBend;
	updateFreq();
}

void LuathVoice::setCutOff(float freqRate, float cutOffGain)
{
	float freq = mCalculatedFreq * freqRate;
	mCutOffFilter.setHighshelfParam(sampleFreq(), freq, 1.f, cutOffGain);
}
void LuathVoice::setResonance(float freqRate, float overtoneGain)
{
	float freq = mCalculatedFreq * freqRate;
	mResonanceFilter.setPeakingParam(sampleFreq(), freq, 1.f, overtoneGain);
}
LuathVoice::EnvelopeGenerator& LuathVoice::envolopeGenerator() noexcept
{
	return mEG;
}

void LuathVoice::updateFreq()noexcept
{
	// TODO いずれ平均律以外にも対応したい
	mCalculatedFreq = 440 * exp2((mNoteNo + mPitchBend - 69.0f) / 12.0f);
}