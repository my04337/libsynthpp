#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Generator/FunctionGenerator.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>
#include <LSP/Synth/Voice.hpp>


#include <array>

namespace LSP::Synth
{

// 単純なボイス実装
class SimpleVoice
	: public Voice
{
public:
	using FunctionGenerator = LSP::Generator::FunctionGenerator<float>;

public:
	SimpleVoice(const EnvelopeGenerator& eg, uint32_t noteNo, float pitchBend, float volume)
		: mEG(eg)
		, mNoteNo(noteNo)
		, mPitchBend(pitchBend)
		, mVolume(volume)
	{
		updateFunctionGenerator();
		mEG.noteOn();
	}

	virtual ~SimpleVoice() {}

	virtual float update()override
	{
		auto v = mFG.update();
		v *= mEG.update();
		v *= mVolume;
		return v;
	}
	virtual void setPitchBend(float pitchBend)override 
	{
		mPitchBend = pitchBend;
		updateFunctionGenerator();
	}
	virtual EnvelopeGenerator& envolopeGenerator()override 
	{ 
		return mEG; 
	}
private:
	void updateFunctionGenerator()
	{
		float freq = 440 * exp2((static_cast<float>(mNoteNo) + mPitchBend - 69.0f) / 12.0f);
		mFG.setSinWave(mEG.sampleFreq(), freq, true);
	}

private:
	FunctionGenerator mFG;
	EnvelopeGenerator mEG;
	uint32_t mNoteNo;
	float mPitchBend;
	float mVolume;
};
}