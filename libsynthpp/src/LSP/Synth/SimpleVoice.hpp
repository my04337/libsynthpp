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
	SimpleVoice(const FunctionGenerator& fg, const EnvelopeGenerator& eg, float volume)
		: mFG(fg)
		, mEG(eg)
		, mVolume(volume)
	{
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
	virtual EnvelopeGenerator& envolopeGenerator()override 
	{ 
		return mEG; 
	}

private:
	FunctionGenerator mFG;
	EnvelopeGenerator mEG;
	float mVolume;
};
}