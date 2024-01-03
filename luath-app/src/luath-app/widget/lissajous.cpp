/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/lissajous.hpp>

using namespace luath::app::widget;

Lissajous::Lissajous()
{
	setParams(1.f, 1.f);
}

Lissajous::~Lissajous()
{
}

void Lissajous::setParams(float sampleFreq, float span)
{
	require(sampleFreq > 0);
	require(span > 0);

	setSpan(sampleFreq, span);
}

void Lissajous::onDrawStaticElements(juce::Graphics& g, const int width, const int height, Params& params)
{
	// 罫線
	g.setColour(juce::Colour::fromFloatRGBA(0.5f, 1.f, 0.125f, 1.f));
	for(int i = 1; i <= 9; ++i) {
		float x = width * 0.1f * i;
		float y = height * 0.1f * i;
		g.drawLine(0, y, width, y);
		g.drawLine(x, 0, x, height);
	}

	// 枠
	g.setColour(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 1.f));
	g.drawRect(juce::Rectangle<int>{0, 0, width, height});
}

void Lissajous::onDrawDynamicElements(juce::Graphics& g, const int width, const int height, Params& params, std::array<std::vector<float>, 2>& buffer)
{
	const float midX = width / 2;
	const float midY = height / 2;
	const size_t bufferSize = buffer[0].size();

	// 枠の内側に描画されるようにクリッピング
	juce::Path clipPath;
	auto clipRect = juce::Rectangle<int>(0, 0, width, height).expanded(-1);
	g.reduceClipRegion(clipRect);


	// リサージュ曲線
	auto getPoint = [&](size_t pos) -> juce::Point<float> {
		auto ch1 = buffer[0][pos];
		auto ch2 = buffer[1][pos];
		return {
			midX + width / 2.0f * normalize(ch1),
			midY - height / 2.0f * normalize(ch2),
		};
		};
	juce::Path signalPath;
	signalPath.startNewSubPath(getPoint(0));
	for(size_t i = 1; i < bufferSize; ++i) {
		auto cur = getPoint(i);
		if(cur.getDistanceFrom(signalPath.getCurrentPosition()) >= 1.f) {
			signalPath.lineTo(cur);
		}
	}
	g.setColour(juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 1.f));
	g.strokePath(signalPath, juce::PathStrokeType(1));
}