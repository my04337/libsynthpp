/**
	luath-plugin

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-plugin/plugin/plugin.hpp>

using namespace luath::plugin;

LuathSynthPlugin::LuathSynthPlugin()
	: AudioProcessor(BusesProperties()
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
	)
{
}

LuathSynthPlugin::~LuathSynthPlugin() = default;

synth::LuathSynth& LuathSynthPlugin::synth()noexcept
{
    return mSynth;
}

//==============================================================================

const juce::String LuathSynthPlugin::getName() const
{
    static const juce::String name {L"luath"};
    return name;
}

bool LuathSynthPlugin::acceptsMidi() const
{
    // MIDIメッセージ : 要求する
    return true;
}

bool LuathSynthPlugin::producesMidi() const
{
    // MIDIメッセージ : 生成しない
    return false;
}

bool LuathSynthPlugin::isMidiEffect() const
{
    // MIDIエフェクトプラグインではない
    return false;
}

double LuathSynthPlugin::getTailLengthSeconds() const
{
    return 0.0; 
}

int LuathSynthPlugin::getNumPrograms()
{
    return 1;  // dummy
}

int LuathSynthPlugin::getCurrentProgram()
{
    return 0; // dummy
}

void LuathSynthPlugin::setCurrentProgram(int index)
{
    // nop
}

const juce::String LuathSynthPlugin::getProgramName(int index)
{
    return {}; // dummy
}

void LuathSynthPlugin::changeProgramName(int index, const juce::String& newName)
{
    // nop
}

//==============================================================================

void LuathSynthPlugin::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSynth.setCurrentPlaybackSampleRate(sampleRate);
}

void LuathSynthPlugin::releaseResources()
{
    // nop
}

bool LuathSynthPlugin::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if(layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }
    return true;
}

void LuathSynthPlugin::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    mSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================

bool LuathSynthPlugin::hasEditor() const
{
    return false; 
}

juce::AudioProcessorEditor* LuathSynthPlugin::createEditor()
{
    return nullptr;
}

//==============================================================================
void LuathSynthPlugin::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LuathSynthPlugin::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}