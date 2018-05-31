#pragma once

#include <Luath/Base/Base.hpp>
#include <LSP/MIDI/Synthesizer/Tone.hpp>
#include <LSP/Generator/FunctionGenerator.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>


#include <array>

namespace Luath::Synthesizer
{
using ToneId = LSP::MIDI::Synthesizer::ToneId;

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

// 鍵盤楽器型トーン
class KeyboardTone
	: public Tone
{
public:
	using FunctionGenerator = LSP::Generator::FunctionGenerator<float>;

public:
	KeyboardTone(const FunctionGenerator& fg, const LSP::Filter::EnvelopeGenerator<float>& channelEG, float toneVolume);

	virtual ~KeyboardTone() {}

	virtual float update()override;
	virtual EnvelopeGenerator& envolopeGenerator()override { return mEG; }

private:
	FunctionGenerator mFG;
	EnvelopeGenerator mEG;
	float mToneVolume;
};

}