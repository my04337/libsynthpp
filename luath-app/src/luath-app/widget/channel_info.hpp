/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/abstract_signal_component.hpp>
#include <lsp/synth/synth.hpp>

namespace luath::app::widget
{

class ChannelInfo final
	: public AbstractDrawableComponent
{
public:
	ChannelInfo();
	~ChannelInfo();

	// 表示パラメータを指定します
	void update(const std::shared_ptr<const lsp::synth::LuathSynth::Digest>& digest);

protected:
	// 描画時にコールバックされます
	void onDrawElements(juce::Graphics& g, int width, int height, Params& params)override;

private:
	juce::Font mFont;
};

} // namespace luath::app::widget