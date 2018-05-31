#include <Luath/Syntesizer/Tones.hpp>

using namespace LSP;
using namespace Luath::Synthesizer;

KeyboardTone::KeyboardTone(const FunctionGenerator& fg, const LSP::Filter::EnvelopeGenerator<float>& channelEG, float toneVolume)
	: mFG(fg)
	, mEG(channelEG)
	, mToneVolume(toneVolume)
{

}

float KeyboardTone::update()
{
	auto v = mFG.update();
	v *= mEG.update();
	v *= mToneVolume;
	return v;
}