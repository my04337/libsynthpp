/**
	luath-plugin

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-plugin/plugin/processor.hpp>
#include <luath-plugin/plugin/editor.hpp>

using namespace luath::plugin;

LuathSynthProcessorEditor::LuathSynthProcessorEditor(LuathSynthProcessor& processor)
	: AudioProcessorEditor(&processor)
    , mProcessor(processor)
{
}

LuathSynthProcessorEditor::~LuathSynthProcessorEditor() = default;

//==============================================================================
void LuathSynthProcessorEditor::paint(juce::Graphics&)
{
	// TODO implementation
}
void LuathSynthProcessorEditor::resized() 
{
	// TODO implementation
}