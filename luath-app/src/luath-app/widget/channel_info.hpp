﻿// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/base_component.hpp>
#include <lsp/synth/synth.hpp>

namespace luath::app::widget
{

class ChannelInfo final
	: public BaseComponent
{
public:
	ChannelInfo();
	~ChannelInfo();

	// 表示パラメータを指定します
	void update(const std::shared_ptr<const lsp::synth::LuathSynth::Digest>& digest);

protected:
	// 描画時にコールバックされます
	void onRendering(juce::Graphics& g, int width, int height, Params& params)override;

private:
	juce::Font mFont;
};

} // namespace luath::app::widget