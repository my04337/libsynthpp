#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Generator/WaveTableGenerator.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>
#include <LSP/Synth/Voice.hpp>


#include <array>

namespace LSP::Synth
{

// 波形メモリ ボイス実装
class WaveTableVoice
	: public Voice
{
public:
	using WaveTableGenerator = LSP::Generator::WaveTableGenerator<float>;

public:
	WaveTableVoice(size_t sampleFreq, SignalView<float> waveTable, const EnvelopeGenerator& eg, uint32_t noteNo, float pitchBend, float volume)
		: mSampleFreq(sampleFreq)
		, mWG(waveTable)
		, mEG(eg)
		, mNoteNo(noteNo)
		, mPitchBend(pitchBend)
		, mVolume(volume)
	{
		updateFreq();
		mEG.noteOn();
	}

	virtual ~WaveTableVoice() {}

	virtual Info info()const override
	{
		Info info;

		info.freq = mCalculatedFreq;
		info.envelope = mEG.envelope();
		info.state = mEG.state();

		return info;
	}
	virtual float update()override
	{
		auto v = mWG.update(mSampleFreq, mCalculatedFreq);
		v *= mEG.update();
		v *= mVolume;
		return v;
	}
	virtual void setPitchBend(float pitchBend)override 
	{
		mPitchBend = pitchBend;
		updateFreq();
	}
	virtual EnvelopeGenerator& envolopeGenerator()override 
	{ 
		return mEG; 
	}
private:
	void updateFreq()
	{
		mCalculatedFreq = 440 * exp2((static_cast<float>(mNoteNo) + mPitchBend - 69.0f) / 12.0f);
	}

private:
	const size_t mSampleFreq;
	WaveTableGenerator mWG;
	EnvelopeGenerator mEG;
	uint32_t mNoteNo;
	float mPitchBend;
	float mCalculatedFreq = 0;
	float mVolume;
};
}