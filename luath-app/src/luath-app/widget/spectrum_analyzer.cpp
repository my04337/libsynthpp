/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/spectrum_analyzer.hpp>
#include <lsp/dsp/fft.hpp>
#include <lsp/util/thread_pool.hpp>

#include <bit>
#include <array>

using namespace luath::app::widget;

SpectrumAnalyzer::SpectrumAnalyzer()
{
	setParams(1.f, 256);

	mDrawingThead = std::jthread([this](std::stop_token stopToken) {renderThreadMain(stopToken); });
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
	// 描画スレッドを安全に停止
	if(mDrawingThead.joinable()) {
		mDrawingThead.request_stop();
		mDrawingThead.join();
	}
}
void SpectrumAnalyzer::setParams(float sampleFreq, size_t bufferSize, uint32_t strechRate)
{
	require(sampleFreq > 0);
	require(bufferSize > 0 && std::has_single_bit(bufferSize));
	require(strechRate >= 1 && std::has_single_bit(strechRate));

	auto span = static_cast<float>(bufferSize / sampleFreq);
	require(span > 0);

	std::lock_guard lock(mInputMutex);
	mSampleFreq = sampleFreq;
	mStrechRate = strechRate;

	for(auto& buffer : mInputBuffer) buffer.resize(bufferSize, 0.f);
}

void SpectrumAnalyzer::write(const Signal<float>& sig)
{
	using std::views::iota;
	using std::views::zip;

	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_samples = sig.samples();

	require(signal_channels == 2, "SpectrumAnalyzer : write - failed (channel count is mismatch)");

	for(auto&& [ch, buffer] : zip(iota(0), mInputBuffer)) {
		// バッファ末尾に追記
		buffer.insert(buffer.end(), sig.data(ch), sig.data(ch) + signal_samples);

		// リングバッファとして振る舞うため、先頭から同じサイズを削除
		buffer.erase(buffer.begin(), buffer.begin() + signal_samples);
	}
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
	// 描画済の画像をそのまま出力する
	{
		std::lock_guard lock(mDrawingMutex);
		for(auto& drawnImage : mDrawnImages) g.drawImageAt(drawnImage, getX(), getY());

		// 描画パラメータ更新
		mComponentWidthForDrawing = getWidth();
		mComponentHeightForDrawing = getHeight();
		mScaleFactorForDrawing = getApproximateScaleFactorForComponent(this);
	}
	// 描画リクエストを発行
	mRequestDrawEvent.set();
}

void SpectrumAnalyzer::renderThreadMain(std::stop_token stopToken)
{
	using std::views::iota;
	using std::views::zip;

	// 描画用信号バッファ
	std::array<std::vector<float>, 2> drawingSignalBuffer;
	std::array<std::vector<float>, 2> drawingFftRealBuffer;
	std::array<std::vector<float>, 2> drawingFftImageBuffer;
	std::vector<float> drawingFftWindowShape;

	// 静的な表示部分の描画キャッシュ
	juce::Image cachedStaticImage;

	// 左右チャネル分の動的部分の描画を高速化するためのスレッドプール
	ThreadPool threadPoolForDynamicImage(2);

	// 描画ループ 
	while(mRequestDrawEvent.try_wait(stopToken, std::chrono::milliseconds(10))) {
		// 信号出力をブロックしないように描画用信号バッファへコピー
		juce::Rectangle<int> unscaledCanvasRect;
		float sampleFreq;
		uint32_t strechRate;
		float scaleFactor;
		{
			std::lock_guard lock(mInputMutex);

			sampleFreq = mSampleFreq;
			strechRate = mStrechRate;

			for(auto&& [input, drawing, real, image] : zip(mInputBuffer, drawingSignalBuffer, drawingFftRealBuffer, drawingFftImageBuffer)) {
				drawing.resize(input.size());
				real.resize(input.size() * strechRate);
				image.resize(input.size() * strechRate);

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

		const auto buffer_size = drawingSignalBuffer[0].size();
		const auto scale_rate = strechRate;
		const float frequency_resolution = static_cast<float>(sampleFreq) / static_cast<float>(buffer_size); // 周波数分解能

		const auto& rect = unscaledCanvasRect;
		const auto scaledCanvasRect = unscaledCanvasRect * scaleFactor;

		// よく使われる定数群
		static const float log_min_freq = std::log10f(20); // Hz
		static const float log_max_freq = std::log10f(22000); // Hz
		static const float log_min_dbfs = -20; // dbFS(power)
		static const float log_max_dbfs = +60; // dbFS(power)
		static const float horizontal_resolution = 0.5f; // px ※高周波部分の描画間引用

		if(drawingFftWindowShape.size() != buffer_size) {
			drawingFftWindowShape.resize(buffer_size, 0.f);
			for(size_t i = 0; i < buffer_size; ++i) {
				drawingFftWindowShape[i] = lsp::dsp::fft::HammingWf(i / (float)buffer_size);
			}
		}

		// 対数軸への変換関数
		auto power2vert = [&](float power)->float {
			if(power < 1.0e-8f) power = 1.0e-8f; // オーバーフロー対策の補正
			float dbfs = 10 * std::log10f(power);

			return std::clamp(height - (dbfs - log_min_dbfs) / (log_max_dbfs - log_min_dbfs) * height, 0.f, height - 1.f);
			};
		auto freq2horz = [&](float freq)->float {
			if(freq < 1.0e-8f) freq = 1.0e-8f; // オーバーフロー対策の補正
			float f = std::log10(freq);

			return std::clamp((f - log_min_freq) / (log_max_freq - log_min_freq) * width, 0.f, width - 1.f);
			};
		auto horz2freq = [&](float x)->float {
			// freq2horzの逆変換を行う
			return powf(10.f, x / width * (log_max_freq - log_min_freq) + log_min_freq);
			};


		// 静的部分の描画開始
		if(cachedStaticImage.getWidth() != scaledCanvasRect.getWidth() || cachedStaticImage.getHeight() != scaledCanvasRect.getHeight()) {
			cachedStaticImage = juce::Image(juce::Image::ARGB, scaledCanvasRect.getWidth(), scaledCanvasRect.getHeight(), true);
			juce::Graphics g(cachedStaticImage);
			g.addTransform(juce::AffineTransform::scale(scaleFactor));

			// グラフ目盛り描画 - 薄い区切り線
			juce::Path scalePath;
			for(float digit = 1; digit < 5; ++digit) {
				float base_freq = std::powf(10, digit);
				for(int i = 1; i < 10; ++i) {
					float x = left + freq2horz(base_freq * i);
					scalePath.startNewSubPath(x, top);
					scalePath.lineTo(x, bottom - 1.f);
				}
			}
			for(float digit = log_min_dbfs / 10; digit < log_max_dbfs / 10; ++digit) {
				float base_v = std::powf(10, digit);
				for(int i = 1; i < 10; ++i) {
					float y = top + power2vert(base_v * i);
					scalePath.startNewSubPath(left, y);
					scalePath.lineTo(right - 1.f, y);
				}
			}
			g.setColour(juce::Colour::fromFloatRGBA(0.5f, 1.f, 0.125f, 1.f));
			g.strokePath(scalePath, juce::PathStrokeType(1));

			// グラフ目盛り描画 - 濃い区切り線
			scalePath.clear();
			for(float digit = 1; digit < 5; ++digit) {
				float base_freq = std::powf(10, digit);
				float x = left + freq2horz(base_freq);
				scalePath.startNewSubPath(x, top);
				scalePath.lineTo(x, bottom - 1.f);
			}
			for(float digit = log_min_dbfs / 10; digit < log_max_dbfs / 10; ++digit) {
				float base_v = std::powf(10, digit);
				float y = top + power2vert(base_v);
				scalePath.startNewSubPath(left, y);
				scalePath.lineTo(right - 1.f, y);
			}
			g.setColour(juce::Colour::fromFloatRGBA(0.25f, 0.75f, 0.125f, 1.f));
			g.strokePath(scalePath, juce::PathStrokeType(1));

			// 枠描画
			g.setColour(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 1.f));
			g.drawRect(rect);
		}

		// 左右チャネルの動的部分の描画
		static const std::array<juce::Colour, 2> channelColor{
			juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 0.5f),
			juce::Colour::fromFloatRGBA(0.f, 0.f, 1.f, 0.5f),
		};
		std::array<std::future<juce::Image>, 2> dynamicImageFuture;
		for(auto&& [drawingBuffer, real, image, color, future] : zip(drawingSignalBuffer, drawingFftRealBuffer, drawingFftImageBuffer, channelColor, dynamicImageFuture))
		{
			future = threadPoolForDynamicImage.enqueue([&] {
				juce::Image drawnImage = juce::Image(juce::Image::ARGB, scaledCanvasRect.getWidth(), scaledCanvasRect.getHeight(), true);
				juce::Graphics g(drawnImage);
				g.addTransform(juce::AffineTransform::scale(scaleFactor));

				// 枠の内側に描画されるようにクリッピング
				juce::Path clipPath;
				auto clipRect = rect.expanded(-1);
				clipPath.addRectangle(clipRect);
				g.reduceClipRegion(clipPath);

				// FFT実施
				auto& window = drawingFftWindowShape;
				if(strechRate > 1) {
					// 高速な解析のため、信号前後を0パディングしてかさ増しする手法があるためこれを採用した
					// ここでは、ハミング窓をかけることで、信号の端を0に近づける
					// 参考 : https://watlab-blog.com/2020/11/16/zero-padding-fft/
					const size_t window_offset = buffer_size * (strechRate - 1) / 2;
					std::fill(real.begin(), real.end(), 0.f);
					for(size_t i = 0; i < drawingBuffer.size(); ++i) {
						real[window_offset + i] = drawingBuffer[i] * window[i];
					}
				}
				else {
					// 通常のFFT
					for(size_t i = 0; i < drawingBuffer.size(); ++i) {
						real[i] = drawingBuffer[i] * window[i];
					}
				}
				std::fill(image.begin(), image.end(), 0.f);
				lsp::dsp::fft::fft1d<float>(real.data(), image.data(), static_cast<int>(real.size()), 0, false);

				// 各点の位置を求める
				auto getPoint = [&](size_t pos) -> juce::Point<float> {
					float x = left + freq2horz(pos * frequency_resolution / scale_rate);
					float power = (real[pos] * real[pos] + image[pos] * image[pos]);
					float y = top + power2vert(power);
					return { x, y };
					};
				juce::Path signalPath;
				signalPath.startNewSubPath(getPoint(0));
				float yPeak = bottom;
				for(size_t i = 1; i < real.size() / 2; ++i) { // FFT結果の実軸部分の内、データ後半は折り返し雑音のため使用しない
					auto pt = getPoint(i);
					yPeak = std::min(yPeak, pt.y);
					if(pt.x - signalPath.getCurrentPosition().x >= horizontal_resolution) {
						pt.y = yPeak;
						signalPath.lineTo(pt);
						yPeak = bottom;
					}
				}
				// 描画
				g.setColour(color);
				g.strokePath(signalPath, juce::PathStrokeType(1));

				return drawnImage;
			});
		}


		// 分割描画した物を統合
		std::array<juce::Image, 3> drawnImages {
			cachedStaticImage,
			dynamicImageFuture[0].get(),
			dynamicImageFuture[1].get(),
		};

		// 描画結果を転送
		{
			std::lock_guard lock(mDrawingMutex);
			mDrawnImages = std::move(drawnImages);
		}
	}
}