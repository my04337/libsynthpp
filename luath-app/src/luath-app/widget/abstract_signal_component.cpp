/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/abstract_signal_component.hpp>

using namespace luath::app::widget;
using namespace std::string_literals;
using namespace std::string_view_literals;

AbstractSignalComponent::AbstractSignalComponent()
{
	setSpan(1.f, 1.f);
}

AbstractSignalComponent::~AbstractSignalComponent()
{
}
void AbstractSignalComponent::setSpan(float sampleFreq, float span)
{
	require(sampleFreq > 0);
	require(span > 0);

	auto bufferSize = static_cast<size_t>(sampleFreq * span);

	setParam("sample_freq"s, std::make_any<float>(sampleFreq));
	setParam("buffer_size"s, std::make_any<size_t>(bufferSize));
	setParam("span"s, std::make_any<float>(span));

	for(auto& buffer : mInputBuffer) buffer.resize(bufferSize, 0.f);
}

void AbstractSignalComponent::setBufferSize(float sampleFreq, size_t bufferSize)
{
	require(sampleFreq > 0);
	require(bufferSize > 0);

	setParam("sample_freq"s, std::make_any<float>(sampleFreq));
	setParam("buffer_size"s, std::make_any<size_t>(bufferSize));
	unsetParam("span"s); // 古い値が残っていると誤動作の元なので削除

	for(auto& buffer : mInputBuffer) buffer.resize(bufferSize, 0.f);
}

void AbstractSignalComponent::write(const Signal<float>& sig)
{
	using std::views::iota;
	using std::views::zip;

	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_samples = sig.samples();

	require(signal_channels == 2, "write - failed (channel count is mismatch)");

	for(auto&& [ch, buffer] : zip(iota(0), mInputBuffer)) {
		// バッファ末尾に追記
		buffer.insert(buffer.end(), sig.data(ch), sig.data(ch) + signal_samples);

		// リングバッファとして振る舞うため、先頭から同じサイズを削除
		buffer.erase(buffer.begin(), buffer.begin() + signal_samples);
	}
}

void AbstractSignalComponent::onDrawElements(juce::Graphics& g, int width, int height, Params& params)
{
	using std::views::zip;

	// 信号出力をブロックしないように描画用信号バッファへコピー
	std::array<std::vector<float>, 2> drawingBuffer;
	{
		std::lock_guard lock(mInputMutex);

		for(auto&& [input, drawing] : zip(mInputBuffer, drawingBuffer)) {
			drawing.resize(input.size());
			std::copy(input.begin(), input.end(), drawing.begin());
		}

	}
	const auto get_any_or = [&params]<class value_type>(std::string_view key, value_type && value) 
	{
		return lsp::get_any_or(params, key, std::forward<value_type>(value));
	};

	const auto scaleFactor = get_any_or("scale_factor"sv, 1.f);
	const auto scaledWidth = get_any_or("scaled_width"sv, 0);
	const auto scaledHeight = get_any_or("scaled_height"sv, 0);

	const juce::Rectangle<int> unscaledCanvasRect = juce::Rectangle<int>{ 0, 0, width, height };
	const juce::Rectangle<int> scaledCanvasRect = juce::Rectangle<int>{ 0, 0, scaledWidth, scaledHeight };

	// 静的部分の描画開始
	if(mCachedStaticImage.getWidth() != scaledCanvasRect.getWidth() || mCachedStaticImage.getHeight() != scaledCanvasRect.getHeight()) {
		mCachedStaticImage = juce::Image(juce::Image::ARGB, scaledCanvasRect.getWidth(), scaledCanvasRect.getHeight(), true);

		juce::Graphics g(mCachedStaticImage);
		g.addTransform(juce::AffineTransform::scale(scaleFactor));

		if(unscaledCanvasRect.getWidth() > 0 && unscaledCanvasRect.getHeight() > 0) {
			onDrawStaticElements(g, unscaledCanvasRect.getWidth(), unscaledCanvasRect.getHeight(), params);
		}
	}

	// 動的部分の描画開始
	{
		g.drawImage(mCachedStaticImage, 0, 0, width, height, 0, 0, scaledWidth, scaledHeight);

		if(unscaledCanvasRect.getWidth() > 0 && unscaledCanvasRect.getHeight() > 0) {
			onDrawDynamicElements(g, unscaledCanvasRect.getWidth(), unscaledCanvasRect.getHeight(), params, drawingBuffer);
		}
	}
}