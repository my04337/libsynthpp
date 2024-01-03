/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/abstract_signal_component.hpp>

namespace luath::app::widget
{

class OscilloScope final
	: public AbstractSignalComponent
{
public:
	OscilloScope();
	~OscilloScope();

	// 表示パラメータを指定します
	void setParams(float sampleFreq, float span);

protected:
	void onDrawStaticElements(juce::Graphics& g, int width, int height, Params& params)override;
	void onDrawDynamicElements(juce::Graphics& g, int width, int height, Params& params, std::array<std::vector<float>, 2>& buffer)override;
};


//
}