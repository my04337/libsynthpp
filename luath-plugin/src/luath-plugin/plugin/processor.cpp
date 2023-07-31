/**
	luath-plugin

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-plugin/plugin/processor.hpp>
#include <luath-plugin/plugin/editor.hpp>

using namespace luath::plugin;

LuathSynthProcessor::LuathSynthProcessor()
	: AudioProcessor(BusesProperties()
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
	)
{
}

LuathSynthProcessor::~LuathSynthProcessor() = default;

synth::LuathSynth& LuathSynthProcessor::synth()noexcept
{
    return mSynth;
}

//==============================================================================

const juce::String LuathSynthProcessor::getName() const
{
    static const juce::String name {L"luath"};
    return name;
}

bool LuathSynthProcessor::acceptsMidi() const
{
    // MIDIメッセージ : 要求する
    return true;
}

bool LuathSynthProcessor::producesMidi() const
{
    // MIDIメッセージ : 生成しない
    return false;
}

bool LuathSynthProcessor::isMidiEffect() const
{
    // MIDIエフェクトプラグインではない
    return false;
}

double LuathSynthProcessor::getTailLengthSeconds() const
{
    return 0.0; 
}

int LuathSynthProcessor::getNumPrograms()
{
    return 1;  // dummy
}

int LuathSynthProcessor::getCurrentProgram()
{
    return 0; // dummy
}

void LuathSynthProcessor::setCurrentProgram(int index)
{
    // nop
}

const juce::String LuathSynthProcessor::getProgramName(int index)
{
    return {}; // dummy
}

void LuathSynthProcessor::changeProgramName(int index, const juce::String& newName)
{
    // nop
}

//==============================================================================

void LuathSynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSynth.setCurrentPlaybackSampleRate(sampleRate);
}

void LuathSynthProcessor::releaseResources()
{
    // nop
}

bool LuathSynthProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if(layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }
    return true;
}

void LuathSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    mSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================

bool LuathSynthProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* LuathSynthProcessor::createEditor()
{
    return new LuathSynthProcessorEditor(*this);
}

//==============================================================================
void LuathSynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LuathSynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}