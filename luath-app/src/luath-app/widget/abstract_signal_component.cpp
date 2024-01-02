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

	mDrawingThead = std::jthread([this](std::stop_token stopToken) {drawingThreadMain(stopToken); });
}

AbstractSignalComponent::~AbstractSignalComponent()
{
	// 描画スレッドを安全に停止
	if(mDrawingThead.joinable()) {
		mDrawingThead.request_stop();
		mDrawingThead.join();
	}
}
void AbstractSignalComponent::setSpan(float sampleFreq, float span)
{
	require(sampleFreq > 0);
	require(span > 0);

	auto bufferSize = static_cast<size_t>(sampleFreq * span);

	mExtras.insert_or_assign("sample_freq"s, std::make_any<float>(sampleFreq));
	mExtras.insert_or_assign("buffer_size"s, std::make_any<size_t>(bufferSize));
	mExtras.insert_or_assign("span"s, std::make_any<float>(span));

	for(auto& buffer : mInputBuffer) buffer.resize(bufferSize, 0.f);
}

void AbstractSignalComponent::setBufferSize(float sampleFreq, size_t bufferSize)
{
	require(sampleFreq > 0);
	require(bufferSize > 0);

	mExtras.insert_or_assign("sample_freq"s, std::make_any<float>(sampleFreq));
	mExtras.insert_or_assign("buffer_size"s, std::make_any<size_t>(bufferSize));
	mExtras.erase("span"s); // 古い値が残っていると誤動作の元なので削除

	for(auto& buffer : mInputBuffer) buffer.resize(bufferSize, 0.f);
}
void AbstractSignalComponent::setExtra(std::string_view key, std::any&& value)
{
	std::lock_guard lock(mInputMutex);
	mExtras.insert_or_assign(std::string(key), std::move(value));

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

void AbstractSignalComponent::paint(juce::Graphics& g)
{
	// 以前に描画した画像を出力する
	{
		std::lock_guard lock(mDrawingMutex);

		// スレッドセーフに新たな描画サイズを記録
		int width = getWidth();
		int height = getHeight();
		float scaleFactor = g.getInternalContext().getPhysicalPixelScaleFactor();
		int scaledWidth = static_cast<int>(width * scaleFactor);
		int scaledHeight = static_cast<int>(height * scaleFactor);

		mExtras.insert_or_assign("width"s, std::make_any<int>(width));
		mExtras.insert_or_assign("height"s, std::make_any<int>(height));
		mExtras.insert_or_assign("scale_factor"s, std::make_any<float>(scaleFactor));

		// 丸め方が混在すると混乱を招くため、スケール後のサイズ(整数値)もここで定義する
		mExtras.insert_or_assign("scaled_width"s, scaledWidth);
		mExtras.insert_or_assign("scaled_height"s, scaledHeight);

		// 描画済画像を出力
		g.drawImage(mDrawnImage, getX(), getY(), width, height, 0, 0, scaledWidth, scaledHeight);
	}
	// 新たな描画をリクエストする
	mRequestDrawEvent.set();
}
void AbstractSignalComponent::drawingThreadMain(std::stop_token stopToken)
{
	using std::views::zip;

	// 描画用信号バッファ
	std::array<std::vector<float>, 2> drawingBuffer;

	// 静的な表示部分の描画キャッシュ
	juce::Image cachedStaticImage;


	// 描画ループ 
	while(mRequestDrawEvent.try_wait(stopToken, std::chrono::milliseconds(10))) {
		// 信号出力をブロックしないように描画用信号バッファへコピー
		Extras extras;
		{
			std::lock_guard lock(mInputMutex);

			for(auto&& [input, drawing] : zip(mInputBuffer, drawingBuffer)) {
				drawing.resize(input.size());
				std::copy(input.begin(), input.end(), drawing.begin());
			}

			extras = mExtras;
		}
		const auto get_any_or = [&extras]<class value_type>(std::string_view key, value_type && value) 
		{
			return lsp::get_any_or(extras, key, std::forward<value_type>(value));
		};

		const auto width = get_any_or("width"sv, 0);
		const auto height = get_any_or("height"sv, 0);
		const auto scaleFactor = get_any_or("scale_factor"sv, 1.f);

		const auto scaledWidth = get_any_or("scaled_width"sv, 0);
		const auto scaledHeight = get_any_or("scaled_height"sv, 0);

		const juce::Rectangle<int> unscaledCanvasRect = juce::Rectangle<int>{ 0, 0, width, height };
		const juce::Rectangle<int> scaledCanvasRect = juce::Rectangle<int>{ 0, 0, scaledWidth, scaledHeight };

		// 静的部分の描画開始
		if(cachedStaticImage.getWidth() != scaledCanvasRect.getWidth() || cachedStaticImage.getHeight() != scaledCanvasRect.getHeight()) {
			cachedStaticImage = juce::Image(juce::Image::ARGB, scaledCanvasRect.getWidth(), scaledCanvasRect.getHeight(), true);

			juce::Graphics g(cachedStaticImage);
			g.addTransform(juce::AffineTransform::scale(scaleFactor));

			if(unscaledCanvasRect.getWidth() > 0 && unscaledCanvasRect.getHeight() > 0) {
				onDrawStaticElements(g, unscaledCanvasRect.getWidth(), unscaledCanvasRect.getHeight(), extras);
			}
		}

		// 動的部分の描画開始
		juce::Image drawnImage = juce::Image(juce::Image::ARGB, scaledCanvasRect.getWidth(), scaledCanvasRect.getHeight(), true);
		{
			juce::Graphics g(drawnImage);
			g.drawImageAt(cachedStaticImage, 0, 0); // 静的部分はスケーリングされる前に描画
			g.addTransform(juce::AffineTransform::scale(scaleFactor));

			if(unscaledCanvasRect.getWidth() > 0 && unscaledCanvasRect.getHeight() > 0) {
				onDrawDynamicElements(g, unscaledCanvasRect.getWidth(), unscaledCanvasRect.getHeight(), extras, drawingBuffer);
			}
		}

		// 描画結果を転送
		{
			std::lock_guard lock(mDrawingMutex);
			mDrawnImage = std::move(drawnImage);
		}
	}
}