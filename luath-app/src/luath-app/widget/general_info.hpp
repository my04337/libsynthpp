/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/base_component.hpp>
#include <lsp/synth/synth.hpp>

namespace luath::app::widget
{

class GeneralInfo final
	: public BaseComponent
{
public:
	GeneralInfo();
	~GeneralInfo();

	// 表示パラメータを指定します
	void update(const juce::AudioDeviceManager& manager, const std::shared_ptr<const lsp::synth::LuathSynth::Digest>& digest);

protected:
	// 描画時にコールバックされます
	void onRendering(juce::Graphics& g, int width, int height, Params& params)override;

private:
	juce::Font mFont;
};

} // namespace luath::app::widget