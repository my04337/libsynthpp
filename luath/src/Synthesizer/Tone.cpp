#include <Luath/Syntesizer/Tone.hpp>

using namespace LSP;
using namespace Luath::Synthesizer;

SimpleTone::SimpleTone(const FunctionGenerator& fg, const LSP::Filter::EnvelopeGenerator<float>& eg, float toneVolume)
	: mFG(fg)
	, mEG(eg)
	, mToneVolume(toneVolume)
{

}

float SimpleTone::update()
{
	auto v = mFG.update();
	v *= mEG.update();
	v *= mToneVolume;
	return v;
}