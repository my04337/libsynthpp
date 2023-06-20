#include <lsp/synth/voice.hpp>

using namespace lsp;
using namespace lsp::synth;

Voice::Voice(uint32_t sampleFreq, float noteNo, float pitchBend, float volume, bool hold)
	: mSampleFreq(sampleFreq)
	, mNoteNo(noteNo)
	, mPitchBend(pitchBend)
	, mVolume(volume)
	, mHold(hold)
{
	updateFreq();
}

Voice::~Voice() = default;


Voice::Digest Voice::digest()const noexcept
{
	Digest digest;

	digest.freq = mCalculatedFreq;
	digest.envelope = mEG.envelope();
	digest.state = mEG.state();

	return digest;
}
float Voice::noteNo()const noexcept
{
	return mNoteNo;
}

void Voice::noteOff()noexcept
{
	if (mHold) {
		mPendingNoteOff = true;
	} else {
		mPendingNoteOff = false;
		mEG.noteOff();
	}
}
void Voice::noteCut()noexcept
{
	mPendingNoteOff = false;
	mEG.reset();
}
void Voice::setHold(bool hold)noexcept
{
	mHold = hold;
	if (mPendingNoteOff && !hold) {
		noteOff();
	}
}
std::optional<float> Voice::pan()const noexcept
{
	return mPan;
}
void Voice::setPan(float pan)noexcept
{
	mPan = pan;
}
void Voice::setPitchBend(float pitchBend)noexcept
{
	mPitchBend = pitchBend;
	updateFreq();
}

void Voice::setCutOff(float freqRate, float cutOffGain)
{
	float freq = mCalculatedFreq * freqRate;
	mCutOffFilter.setHighshelfParam(mSampleFreq, freq, 1.f, cutOffGain);
}
void Voice::setResonance(float freqRate, float overtoneGain)
{
	float freq = mCalculatedFreq * freqRate;
	mResonanceFilter.setPeakingParam(mSampleFreq, freq, 1.f, overtoneGain);
}
Voice::EnvelopeGenerator& Voice::envolopeGenerator() noexcept
{
	return mEG;
}

void Voice::updateFreq()noexcept
{
	// TODO いずれ平均律以外にも対応したい
	mCalculatedFreq = 440 * exp2((mNoteNo + mPitchBend - 69.0f) / 12.0f);
}