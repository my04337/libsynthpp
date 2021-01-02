#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Generator/FunctionGenerator.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>
#include <LSP/MIDI/Synthesizer/Voice.hpp>


#include <array>

namespace LSP::MIDI::Synthesizer
{

// 単純なボイス実装
template <
	typename sample_type,
	class = std::enable_if_t<
	is_sample_type_v<sample_type>
	>
>
class SimpleVoice
	: public Voice<sample_type>
{
public:
	using FunctionGenerator = LSP::Generator::FunctionGenerator<sample_type>;

public:
	SimpleVoice(const FunctionGenerator& fg, const LSP::Filter::EnvelopeGenerator<sample_type>& eg, sample_type volume)
		: mFG(fg)
		, mEG(eg)
		, mVolume(volume)
	{
		mEG.noteOn();
	}

	virtual ~SimpleVoice() {}

	virtual sample_type update()override
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
	sample_type mVolume;
};
}