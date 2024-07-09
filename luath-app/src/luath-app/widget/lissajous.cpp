// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#include <luath-app/widget/lissajous.hpp>

using namespace luath::app::widget;
using namespace std::string_literals;
using namespace std::string_view_literals;
using std::views::iota;
using std::views::zip;

Lissajous::Lissajous()
{
	setParams(1.f, 1.f);
}

Lissajous::~Lissajous()
{
}

void Lissajous::setParams(float sampleFreq, float span)
{
	lsp_require(sampleFreq > 0);
	lsp_require(span > 0);

	auto bufferSize = static_cast<size_t>(sampleFreq * span);

	setParam("sample_freq"s, std::make_any<float>(sampleFreq));
	setParam("buffer_size"s, std::make_any<size_t>(bufferSize));
	setParam("span"s, std::make_any<float>(span));

	for(auto& buffer : mInputBuffer) buffer.resize(bufferSize, 0.f);
	repaintAsync();
}
void Lissajous::write(const Signal<float>& sig)
{
	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_samples = sig.samples();

	lsp_require(signal_channels == 2);

	for(auto&& [ch, buffer] : zip(iota(0), mInputBuffer)) {
		buffer.insert(buffer.end(), sig.data(ch), sig.data(ch) + signal_samples);
		buffer.erase(buffer.begin(), buffer.begin() + signal_samples);
	}
	repaintAsync();
}

void Lissajous::onRendering(juce::Graphics& g, const int width_, const int height_, Params& params)
{
	// 信号出力をブロックしないように描画用信号バッファへコピー
	std::array<std::vector<float>, 2> buffer;
	{
		std::lock_guard lock(mInputMutex);

		for(auto&& [input, drawing] : zip(mInputBuffer, buffer)) {
			drawing.resize(input.size());
			std::copy(input.begin(), input.end(), drawing.begin());
		}
	}

	// よく使う定数などの計算
	const float width = static_cast<float>(width_);
	const float height = static_cast<float>(height_);
	const float midX = width / 2;
	const float midY = height / 2;
	const size_t bufferSize = buffer[0].size();

	// 背景塗りつぶし
	g.fillAll(juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 1.f));

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
	g.drawRect(juce::Rectangle<int>{0, 0, width_, height_});

	// 枠の内側に描画されるようにクリッピング
	juce::Path clipPath;
	auto clipRect = juce::Rectangle<int>(0, 0, width_, height_).expanded(-1);
	g.reduceClipRegion(clipRect);


	// リサージュ曲線
	auto getPoint = [&](size_t pos) -> juce::Point<float> {
		auto ch1 = buffer[0][pos];
		auto ch2 = buffer[1][pos];
		return {
			midX + width / 2.0f * clamp(ch1),
			midY - height / 2.0f * clamp(ch2),
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