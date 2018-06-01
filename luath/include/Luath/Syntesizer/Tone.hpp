#pragma once

#include <Luath/Base/Base.hpp>
#include <LSP/MIDI/Controller.hpp>
#include <LSP/Generator/FunctionGenerator.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>


#include <array>

namespace Luath::Synthesizer
{
using ToneId = LSP::MIDI::ToneId;

// トーン(あるチャネルの1音) - 基底クラス
class Tone
	: non_copy_move
{
public:
	using EnvelopeGenerator = LSP::Filter::EnvelopeGenerator<float>;
	using Signal = LSP::Signal<float>;

public:
	virtual ~Tone() {}

	virtual float update() = 0;
	virtual EnvelopeGenerator& envolopeGenerator() = 0;

};

// 単純なトーン実装
class SimpleTone
	: public Tone
{
public:
	using FunctionGenerator = LSP::Generator::FunctionGenerator<float>;

public:
	SimpleTone(const FunctionGenerator& fg, const LSP::Filter::EnvelopeGenerator<float>& eg, float toneVolume);

	virtual ~SimpleTone() {}

	virtual float update()override;
	virtual EnvelopeGenerator& envolopeGenerator()override { return mEG; }

private:
	FunctionGenerator mFG;
	EnvelopeGenerator mEG;
	float mToneVolume;
};

}