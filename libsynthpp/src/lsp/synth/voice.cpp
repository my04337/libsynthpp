#include <lsp/synth/voice.hpp>

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
	// HoldまたはSostenutoが有効な場合、実際のリリースを保留し、両方が解除されたタイミングでリリースする
	if (mHold || mSostenuto) {
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
	// Hold解除時 : Sostenutoも無効であれば、保留中のnoteOffを実行する
	if (mPendingNoteOff && !hold && !mSostenuto) {
		noteOff();
	}
}
void Voice::setSostenuto(bool sostenuto)noexcept
{
	mSostenuto = sostenuto;
	// Sostenuto解除時 : Holdも無効であれば、保留中のnoteOffを実行する
	if (mPendingNoteOff && !sostenuto && !mHold) {
		noteOff();
	}
}
bool Voice::isNoteOn()const noexcept
{
	// キーが押下中 = noteOffが保留されておらず、かつEGがリリース/止音状態でない
	return !mPendingNoteOff
		&& mEG.state() != EnvelopeState::Release
		&& mEG.state() != EnvelopeState::Free;
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
void Voice::setPolyPressure(float pressure)noexcept
{
	mPolyPressure = pressure;
}

void Voice::setFilter(float cutoffFreq, float Q)noexcept
{
	// カットオフ周波数がナイキスト周波数の半分以上の場合、フィルタをパススルーにする
	// (ナイキスト周波数以上のカットオフは双二次フィルタでは不安定になる)
	float nyquist = mSampleFreq / 2.f;
	if(cutoffFreq >= nyquist * 0.95f) {
		mFilter.resetParam();
	} else {
		mFilter.setLopassParam(static_cast<float>(mSampleFreq), cutoffFreq, Q);
	}
}
Voice::EnvelopeGenerator& Voice::envelopeGenerator() noexcept
{
	return mEG;
}

void Voice::setBaseReleaseTime(float timeSec)noexcept
{
	mBaseReleaseTimeSec = timeSec;
}
void Voice::setReleaseTimeScale(float scale)noexcept
{
	mEG.setReleaseTime(static_cast<float>(mSampleFreq), std::max(0.001f, mBaseReleaseTimeSec * scale));
}

void Voice::updateFreq()noexcept
{
	// TODO いずれ平均律以外にも対応したい
	mCalculatedFreq = 440 * exp2((mNoteNo + mPitchBend - 69.0f) / 12.0f);
}