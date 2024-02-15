/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/oscilloscope.hpp>

using namespace luath::app::widget;
using namespace std::string_literals;
using namespace std::string_view_literals;
using std::views::iota;
using std::views::zip;

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

	auto bufferSize = static_cast<size_t>(sampleFreq * span);

	setParam("sample_freq"s, std::make_any<float>(sampleFreq));
	setParam("buffer_size"s, std::make_any<size_t>(bufferSize));
	setParam("span"s, std::make_any<float>(span));

	for(auto& buffer : mInputBuffer) buffer.resize(bufferSize, 0.f);
	repaintAsync();
}

void OscilloScope::write(const Signal<float>& sig)
{
	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_samples = sig.samples();

	require(signal_channels == 2, "write - failed (channel count is mismatch)");

	for(auto&& [ch, buffer] : zip(iota(0), mInputBuffer)) {
		buffer.insert(buffer.end(), sig.data(ch), sig.data(ch) + signal_samples);
		buffer.erase(buffer.begin(), buffer.begin() + signal_samples);
	}
	repaintAsync();
}

void OscilloScope::onRendering(juce::Graphics& g, const int width_, const int height_, Params& params)
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
	const float speedRatio = bufferSize / width;

	// 背景塗りつぶし
	g.fillAll(juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 1.f));

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
	g.drawRect(juce::Rectangle<int>(0, 0, width_, height_));

	// 枠の内側に描画されるようにクリッピング
	auto clipRect = juce::Rectangle<int>(0, 0, width_, height_).expanded(-1);
	g.reduceClipRegion(clipRect);

	// 信号描画
	static const std::array<juce::Colour, 2> channelColor{
		juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 0.5f),
		juce::Colour::fromFloatRGBA(0.f, 0.f, 1.f, 0.5f),
	};
	std::vector<float> interpolated(static_cast<size_t>(width));
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
		auto getY = [&](float v) {return midY - height / 2.0f * clamp(v); };
		juce::Path path;
		path.startNewSubPath(0, getY(interpolated[0]));
		for(size_t i = 1; i < interpolated.size(); ++i) {
			path.lineTo(static_cast<float>(i), getY(interpolated[i]));
		}
		g.setColour(color);
		g.strokePath(path, juce::PathStrokeType(1));
	}
}