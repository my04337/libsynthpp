/**
	luath-plugin

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-plugin/core/core.hpp>
#include <lsp/synth/synth.hpp>

namespace luath::plugin
{

class LuathSynthProcessorEditor final
	: public juce::AudioProcessorEditor
{
public:
    LuathSynthProcessorEditor(LuathSynthProcessor& processor);
	~LuathSynthProcessorEditor()override;

    //==============================================================================

	void paint(juce::Graphics&) override;
	void resized() override;

private:
	LuathSynthProcessor& mProcessor;
};

}
