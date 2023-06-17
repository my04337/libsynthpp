#include <LSP/Synth/Voice.hpp>

using namespace LSP;
using namespace LSP::Synth;

Voice::Voice(uint32_t sampleFreq, const EnvelopeGenerator& eg, uint32_t noteNo, float pitchBend, float volume, bool hold)
	: mSampleFreq(sampleFreq)
	, mEG(eg)
	, mNoteNo(noteNo)
	, mPitchBend(pitchBend)
	, mVolume(volume)
	, mHold(hold)
{
	updateFreq();
	mEG.noteOn();
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
uint32_t Voice::noteNo()const noexcept
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
const Voice::EnvelopeGenerator& Voice::envolopeGenerator()const noexcept
{
	return mEG;
}

void Voice::updateFreq()noexcept
{
	// TODO いずれ平均律以外にも対応したい
	mCalculatedFreq = 440 * exp2((static_cast<float>(mNoteNo) + mPitchBend - 69.0f) / 12.0f);
}