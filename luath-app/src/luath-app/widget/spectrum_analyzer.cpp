// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#include <luath-app/widget/spectrum_analyzer.hpp>
#include <lsp/dsp/fft.hpp>
#include <lsp/util/thread_pool.hpp>

#include <bit>
#include <array>

using namespace luath::app::widget;
using namespace std::string_literals;
using namespace std::string_view_literals;
using std::views::iota;
using std::views::zip;

SpectrumAnalyzer::SpectrumAnalyzer()
	: mThreadPoolForFft(2)
{
	setParams(1.f, 256);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
}
void SpectrumAnalyzer::setParams(float sampleFreq, size_t bufferSize, uint32_t stretchRate)
{
	lsp_require(sampleFreq > 0);
	lsp_require(bufferSize > 0 && std::has_single_bit(bufferSize));
	lsp_require(stretchRate >= 1 && std::has_single_bit(stretchRate));

	setParam("sample_freq"s, std::make_any<float>(sampleFreq));
	setParam("buffer_size"s, std::make_any<size_t>(bufferSize));
	setParam("stretch_rate", std::make_any<uint32_t>(stretchRate));

	for(auto& buffer : mInputBuffer) buffer.resize(bufferSize, 0.f);
	repaintAsync();
}
void SpectrumAnalyzer::write(const Signal<float>& sig)
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


// よく使われる定数群
static const float log_min_freq = std::log10f(20); // Hz
static const float log_max_freq = std::log10f(22000); // Hz
static const float log_min_dbfs = -20; // dbFS(power)
static const float log_max_dbfs = +60; // dbFS(power)
static const float horizontal_resolution = 0.5f; // px ※高周波部分の描画間引用

// 対数軸への変換関数
float SpectrumAnalyzer::freq2horz(float width, float freq)
{
	if(freq < 1.0e-8f) freq = 1.0e-8f; // オーバーフロー対策の補正
	float f = std::log10(freq);

	return std::clamp((f - log_min_freq) / (log_max_freq - log_min_freq) * width, 0.f, width - 1.f);
};
float SpectrumAnalyzer::horz2freq(float width, float x) 
{
	// freq2horzの逆変換を行う
	return powf(10.f, x / width * (log_max_freq - log_min_freq) + log_min_freq);
}
float SpectrumAnalyzer::power2vert(float height, float power)
{
	if(power < 1.0e-8f) power = 1.0e-8f; // オーバーフロー対策の補正
	float dbfs = 10 * std::log10f(power);

	return std::clamp(height - (dbfs - log_min_dbfs) / (log_max_dbfs - log_min_dbfs) * height, 0.f, height - 1.f);
}

void SpectrumAnalyzer::onRendering(juce::Graphics& g, const int width_, const int height_, Params& params)
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
	const auto width = static_cast<float>(width_);
	const auto height = static_cast<float>(height_);

	const auto left = 0.f;
	const auto top = 0.f;
	const auto right = width;
	const auto bottom = height;

	const auto get_any_or = [&params]<class value_type>(std::string_view key, value_type && value)
	{
		return lsp::get_any_or(params, key, std::forward<value_type>(value));
	};
	const auto sampleFreq = get_any_or("sample_freq"sv, 1.f);
	const auto stretchRate = get_any_or("stretch_rate"sv, 1ui32);

	const float midX = width / 2;
	const float midY = height / 2;
	const size_t bufferSize = buffer[0].size();
	const float frequencyResolution = static_cast<float>(sampleFreq) / static_cast<float>(bufferSize); // 周波数分解能


	// ウィンドウ関数の再計算 ※必要に応じて
	if(mDrawingFftWindowShapeCache.size() != bufferSize) {
		mDrawingFftWindowShapeCache.resize(bufferSize, 0.f);
		for(size_t i = 0; i < bufferSize; ++i) {
			mDrawingFftWindowShapeCache[i] = lsp::dsp::fft::HammingWf(i / (float)bufferSize);
		}
	}

	// FFTの実施 & パスの算出 (並列度を高めるため早いタイミングで開始している)
	std::array<std::future<juce::Path>, 2> signalPathFuture;
	for(auto&& [drawingBuffer, future] : zip(buffer, signalPathFuture))
	{
		future = mThreadPoolForFft.enqueue([&] {
			std::vector<float> real(bufferSize * stretchRate);
			std::vector<float> image(bufferSize * stretchRate);

			// FFTの実施
			auto& window = mDrawingFftWindowShapeCache;
			if(stretchRate > 1) {
				// 高速な解析のため、信号前後を0パディングしてかさ増しする手法があるためこれを採用した
				// ここでは、ハミング窓をかけることで、信号の端を0に近づける
				// 参考 : https://watlab-blog.com/2020/11/16/zero-padding-fft/
				const size_t windowOffset = bufferSize * (stretchRate - 1) / 2;
				std::fill(real.begin(), real.end(), 0.f);
				for(size_t i = 0; i < drawingBuffer.size(); ++i) {
					real[windowOffset + i] = drawingBuffer[i] * window[i];
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
				float x = left + freq2horz(width, pos * frequencyResolution / stretchRate);
				float power = (real[pos] * real[pos] + image[pos] * image[pos]);
				float y = top + power2vert(height, power);
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

			return signalPath;
		});
	}

	// 背景塗りつぶし
	g.fillAll(juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 1.f));

	// グラフ目盛り描画 - 薄い区切り線
	juce::Path scalePath;
	for(float digit = 1; digit < 5; ++digit) {
		float base_freq = std::powf(10, digit);
		for(int i = 1; i < 10; ++i) {
			float x = left + freq2horz(width, base_freq * i);
			scalePath.startNewSubPath(x, top);
			scalePath.lineTo(x, bottom - 1.f);
		}
	}
	for(float digit = log_min_dbfs / 10; digit < log_max_dbfs / 10; ++digit) {
		float base_v = std::powf(10, digit);
		for(int i = 1; i < 10; ++i) {
			float y = top + power2vert(height, base_v * i);
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
		float x = left + freq2horz(width, base_freq);
		scalePath.startNewSubPath(x, top);
		scalePath.lineTo(x, bottom - 1.f);
	}
	for(float digit = log_min_dbfs / 10; digit < log_max_dbfs / 10; ++digit) {
		float base_v = std::powf(10, digit);
		float y = top + power2vert(height, base_v);
		scalePath.startNewSubPath(left, y);
		scalePath.lineTo(right - 1.f, y);
	}
	g.setColour(juce::Colour::fromFloatRGBA(0.25f, 0.75f, 0.125f, 1.f));
	g.strokePath(scalePath, juce::PathStrokeType(1));

	// 枠描画
	g.setColour(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 1.f));
	g.drawRect(juce::Rectangle<int>(0, 0, width_, height_));

	// 枠の内側に描画されるようにクリッピング
	auto clipRect = juce::Rectangle<int>(0, 0, width_, height_).expanded(-1);
	g.reduceClipRegion(clipRect);

	// スペクトル描画
	static const std::array<juce::Colour, 2> channelColor{
		juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 0.5f),
		juce::Colour::fromFloatRGBA(0.f, 0.f, 1.f, 0.5f),
	};
	for(auto&& [color, future] : zip(channelColor, signalPathFuture))
	{
		juce::Path signalPath = future.get();
		g.setColour(color);
		g.strokePath(signalPath, juce::PathStrokeType(1));
	}
}
