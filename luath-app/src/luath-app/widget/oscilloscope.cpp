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

	mDrawingThead = std::jthread([this](std::stop_token stopToken) {renderThreadMain(stopToken); });
}

OscilloScope::~OscilloScope()
{
	// 描画スレッドを安全に停止
	if(mDrawingThead.joinable()) {
		mDrawingThead.request_stop();
		mDrawingThead.join();
	}
}
void OscilloScope::setParams(float sampleFreq, float span)
{
	require(sampleFreq > 0);
	require(span > 0);

	auto bufferSize = static_cast<size_t>(sampleFreq * span);
	require(bufferSize > 0);

	std::lock_guard lock(mInputMutex);
	mSampleFreq = sampleFreq;

	for(auto& buffer : mInputBuffer) buffer.resize(bufferSize, 0.f);
}

void OscilloScope::write(const Signal<float>& sig)
{
	using std::views::iota;
	using std::views::zip;

	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_samples = sig.samples();

	require(signal_channels == 2, "OscilloScope : write - failed (channel count is mismatch)");

	for(auto&& [ch, buffer] : zip(iota(0), mInputBuffer)) {
		// バッファ末尾に追記
		buffer.insert(buffer.end(), sig.data(ch), sig.data(ch) + signal_samples);

		// リングバッファとして振る舞うため、先頭から同じサイズを削除
		buffer.erase(buffer.begin(), buffer.begin() + signal_samples);
	}
}

void OscilloScope::paint(juce::Graphics& g)
{
	// 描画済の画像をそのまま出力する
	{
		std::lock_guard lock(mDrawingMutex);
		g.drawImageAt(mDrawnImage, getX(), getY());

		// 描画パラメータ更新
		mComponentWidthForDrawing = getWidth();
		mComponentHeightForDrawing = getHeight();
		mScaleFactorForDrawing = getApproximateScaleFactorForComponent(this);
	}
	// 描画リクエストを発行
	mRequestDrawEvent.set();
}

void OscilloScope::renderThreadMain(std::stop_token stopToken)
{
	using std::views::zip;

	// 描画用信号バッファ
	std::array<std::vector<float>, 2> drawingBuffer;
	std::vector<float> interpolatedSignalBuffer;

	// 静的な表示部分の描画キャッシュ
	juce::Image cachedStaticImage;

	// 描画ループ 
	while(mRequestDrawEvent.try_wait(stopToken, std::chrono::milliseconds(10))) {
		// 信号出力をブロックしないように描画用信号バッファへコピー
		juce::Rectangle<int> unscaledCanvasRect;
		float scaleFactor;
		{
			std::lock_guard lock(mInputMutex);

			for(auto&& [input, drawing] : zip(mInputBuffer, drawingBuffer)) {
				drawing.resize(input.size());
				std::copy(input.begin(), input.end(), drawing.begin());
			}

			unscaledCanvasRect = juce::Rectangle<int>{ 0, 0, mComponentWidthForDrawing, mComponentHeightForDrawing };
			scaleFactor = mScaleFactorForDrawing;
		}

		// 描画サイズが0ならなにも描画しない
		if(unscaledCanvasRect.getWidth() <= 0 || unscaledCanvasRect.getHeight() <= 0) return;

		// よく使う値を先に計算
		const auto left = static_cast<float>(unscaledCanvasRect.getX());
		const auto top = static_cast<float>(unscaledCanvasRect.getY());
		const auto right = static_cast<float>(unscaledCanvasRect.getRight());
		const auto bottom = static_cast<float>(unscaledCanvasRect.getBottom());
		const auto width = static_cast<float>(unscaledCanvasRect.getWidth());
		const auto height = static_cast<float>(unscaledCanvasRect.getHeight());

		const float mid_x = (left + right) / 2;
		const float mid_y = (top + bottom) / 2;
		const size_t buffer_size = drawingBuffer[0].size();
		const float speed_ratio = buffer_size / width;

		const auto& rect = unscaledCanvasRect;
		const auto scaledCanvasRect = unscaledCanvasRect * scaleFactor;


		// 静的部分の描画開始
		if(cachedStaticImage.getWidth() != scaledCanvasRect.getWidth() || cachedStaticImage.getHeight() != scaledCanvasRect.getHeight()) {
			cachedStaticImage = juce::Image(juce::Image::ARGB, scaledCanvasRect.getWidth(), scaledCanvasRect.getHeight(), true);
			juce::Graphics g(cachedStaticImage);
			g.addTransform(juce::AffineTransform::scale(scaleFactor));

			// 罫線描画
			g.setColour(juce::Colour::fromFloatRGBA(0.5f, 1.f, 0.125f, 1.f));
			for(int i = 1; i <= 9; ++i) {
				float x = left + width * 0.1f * i;
				float y = top + height * 0.1f * i;
				g.drawLine(left, y, right, y);
				g.drawLine(x, top, x, bottom);
			}

			// 枠描画
			g.setColour(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 1.f));
			g.drawRect(rect);
		}

		// 動的部分の描画開始
		juce::Image drawnImage = juce::Image(juce::Image::ARGB, scaledCanvasRect.getWidth(), scaledCanvasRect.getHeight(), true);
		{
			juce::Graphics g(drawnImage);
			g.drawImageAt(cachedStaticImage, 0, 0); // 静的部分はスケーリングされる前に描画
			g.addTransform(juce::AffineTransform::scale(scaleFactor));

			auto& interpolated = interpolatedSignalBuffer;
			interpolated.resize(drawnImage.getWidth());

			// 枠の内側に描画されるようにクリッピング
			juce::Path clipPath;
			auto clipRect = rect.expanded(-1);
			clipPath.addRectangle(clipRect);
			g.reduceClipRegion(clipPath);

			// 信号描画
			static const std::array<juce::Colour, 2> channelColor{
				juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 0.5f),
				juce::Colour::fromFloatRGBA(0.f, 0.f, 1.f, 0.5f),
			};
			for(auto&& [buffer, color] : zip(drawingBuffer, channelColor)) {
				// そのまま全サンプルを描画すると重いため、描画画素数に応じた値まで間引く
				juce::LinearInterpolator().process(
					speed_ratio,
					buffer.data(),
					interpolated.data(),
					static_cast<int>(interpolated.size()),
					static_cast<int>(buffer_size),
					0
				);

				// 複数回のdrawLine呼び出しは重いため、Pathとして一括で描画する
				auto getY = [&](float v) {return mid_y - height / 2.0f * normalize(v); };
				juce::Path path;
				path.startNewSubPath(left, getY(interpolated[0]));
				for(size_t i = 1; i < interpolated.size(); ++i) {
					path.lineTo(left + static_cast<float>(i), getY(interpolated[i]));
				}
				g.setColour(color);
				g.strokePath(path, juce::PathStrokeType(1));
			}
		}

		// 描画結果を転送
		{
			std::lock_guard lock(mDrawingMutex);
			mDrawnImage = std::move(drawnImage);
		}
	}
}