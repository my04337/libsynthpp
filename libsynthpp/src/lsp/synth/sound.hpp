/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>

namespace lsp::synth
{
class LuathSynth;


// Luath用サウンド 
// MEMO juce::SynthesizerVoiceは何らかのjuce::SynthesizerSoundに属している必要があるため、最低限の物を用意した。
class LuathSound final
	: public juce::SynthesiserSound
{
public:
	LuathSound(LuathSynth& synth, int ch) : mSynth(synth), mMidiChannel(ch) {}

	bool appliesToNote(int midiNoteNumber)override { return true; }
	bool appliesToChannel(int midiChannel)override { return midiChannel == mMidiChannel; }

	LuathSynth& synth()const noexcept { return mSynth; }
	int midiChannel()const noexcept { return mMidiChannel; }

private:
	LuathSynth& mSynth;
	int mMidiChannel;
};

}