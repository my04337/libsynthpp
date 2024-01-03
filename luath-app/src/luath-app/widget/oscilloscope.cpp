/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/oscilloscope.hpp>

using namespace luath::app::widget;

OscilloScope::OscilloScope()
{
	setParams(1.f, 1.f);
}

OscilloScope::~OscilloScope()
{
}
void OscilloScope::setParams(float sampleFreq, float span)
{
	require(sampleFreq > 0);
	require(span > 0);

	setSpan(sampleFreq, span);
}

void OscilloScope::onDrawStaticElements(juce::Graphics& g, const int width, const int height, Params& params)
{
	// 罫線描画
	g.setColour(juce::Colour::fromFloatRGBA(0.5f, 1.f, 0.125f, 1.f));
	for(int i = 1; i <= 9; ++i) {
		float x = width * 0.1f * i;
		float y = height * 0.1f * i;
		g.drawLine(0, y, width, y);
		g.drawLine(x, 0, x, height);
	}

	// 枠描画
	g.setColour(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 1.f));
	g.drawRect(juce::Rectangle<int>(0, 0, width, height));
}

void OscilloScope::onDrawDynamicElements(juce::Graphics& g, const int width, const int height, Params& params, std::array<std::vector<float>, 2>& buffer)
{
	using std::views::zip;

	const float midX = width / 2;
	const float midY = height / 2;
	const size_t bufferSize = buffer[0].size();
	const float speedRatio = bufferSize / width;

	// 間引きのためのバッファ
	std::vector<float> interpolated(static_cast<size_t>(width));

	// 枠の内側に描画されるようにクリッピング
	auto clipRect = juce::Rectangle<int>(0, 0, width, height).expanded(-1);
	g.reduceClipRegion(clipRect);

	// 信号描画
	static const std::array<juce::Colour, 2> channelColor{
		juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 0.5f),
		juce::Colour::fromFloatRGBA(0.f, 0.f, 1.f, 0.5f),
	};
	for(auto&& [buffer, color] : zip(buffer, channelColor)) {
		// そのまま全サンプルを描画すると重いため、描画画素数に応じた値まで間引く
		juce::LinearInterpolator().process(
			speedRatio,
			buffer.data(),
			interpolated.data(),
			static_cast<int>(interpolated.size()),
			static_cast<int>(bufferSize),
			0
		);

		// 複数回のdrawLine呼び出しは重いため、Pathとして一括で描画する
		auto getY = [&](float v) {return midY - height / 2.0f * normalize(v); };
		juce::Path path;
		path.startNewSubPath(0, getY(interpolated[0]));
		for(size_t i = 1; i < interpolated.size(); ++i) {
			path.lineTo( static_cast<float>(i), getY(interpolated[i]));
		}
		g.setColour(color);
		g.strokePath(path, juce::PathStrokeType(1));
	}
}
