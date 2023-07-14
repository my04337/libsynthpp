#include <luath-app/widget/spectrum_analyzer.hpp>
#include <lsp/dsp/fft.hpp>

#include <bit>
#include <array>

using namespace luath::app::widget;

SpectrumAnalyzer::SpectrumAnalyzer()
{
	setParams(1.f, 256);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
}
void SpectrumAnalyzer::setParams(float sampleFreq, size_t bufferSize, uint32_t strechRate)
{
	require(sampleFreq > 0);
	require(bufferSize > 0 && std::has_single_bit(bufferSize));
	require(strechRate > 0 && std::has_single_bit(strechRate));

	auto span = static_cast<float>(bufferSize / sampleFreq);
	require(span > 0);

	std::lock_guard lock(mInputMutex);
	mSampleFreq = sampleFreq;
	mSpan = span;
	mStrechRate = strechRate;
	mUnitBufferSize = bufferSize;

	mInputBuffer1ch.resize(bufferSize, 0.f);
	mInputBuffer2ch.resize(bufferSize, 0.f);
	mDrawingBuffer1ch.resize(bufferSize, 0.f);
	mDrawingBuffer2ch.resize(bufferSize, 0.f);

	mDrawingFftRealBuffer.resize(bufferSize * strechRate, 0.f);
	mDrawingFftImageBuffer.resize(bufferSize * strechRate, 0.f);

	mDrawingFftWindowCache.resize(bufferSize, 0.f);

	// 高速な解析のため、信号前後を0パディングする
	// 参考 : https://watlab-blog.com/2020/11/16/zero-padding-fft/
	for(size_t i = 0; i < bufferSize; ++i) {
		mDrawingFftWindowCache[i] = lsp::dsp::fft::HammingWf(i / (float)bufferSize);
	}
}

void SpectrumAnalyzer::write(const Signal<float>& sig)
{
	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_samples = sig.samples();

	require(signal_channels == 2, "SpectrumAnalyzer : write - failed (channel count is mismatch)");

	// バッファ末尾に追記
	mInputBuffer1ch.insert(mInputBuffer1ch.end(), sig.data(0), sig.data(0) + signal_samples);
	mInputBuffer2ch.insert(mInputBuffer2ch.end(), sig.data(1), sig.data(1) + signal_samples);

	// リングバッファとして振る舞うため、先頭から同じサイズを削除
	mInputBuffer1ch.erase(mInputBuffer1ch.begin(), mInputBuffer1ch.begin() + signal_samples);
	mInputBuffer2ch.erase(mInputBuffer2ch.begin(), mInputBuffer2ch.begin() + signal_samples);
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
	// 信号出力をブロックしないように描画用信号バッファへコピー
	{
		std::lock_guard lock(mInputMutex);
		std::copy(mInputBuffer1ch.begin(), mInputBuffer1ch.end(), mDrawingBuffer1ch.begin());
		std::copy(mInputBuffer2ch.begin(), mInputBuffer2ch.end(), mDrawingBuffer2ch.begin());
	}

	// 描画開始
	g.saveState();
	auto fin_act_restore_state = finally([&] {g.restoreState(); });

	const juce::Rectangle<float>  rect{ static_cast<float>(getX()), static_cast<float>(getY()), static_cast<float>(getWidth()), static_cast<float>(getHeight())};
	juce::Path clipPath;
	clipPath.addRectangle(rect);
	g.reduceClipRegion(clipPath);

	// よく使う値を先に計算
	static const float log_min_freq = std::log10f(20); // Hz
	static const float log_max_freq = std::log10f(22000); // Hz
	static const float log_min_dbfs = -20; // dbFS(power)
	static const float log_max_dbfs = +60; // dbFS(power)
	static const float horizontal_resolution = 0.5f; // px ※高周波部分の描画間引用

	const float left = rect.getX();
	const float top = rect.getY();
	const float right = rect.getRight();
	const float bottom = rect.getBottom();
	const float width = rect.getWidth();
	const float height = rect.getHeight();

	const float mid_x = (left + right) / 2;
	const float mid_y = (top + bottom) / 2;

	const auto scale_rate = mStrechRate;
	const float frequency_resolution = static_cast<float>(mSampleFreq) / static_cast<float>(mUnitBufferSize); // 周波数分解能


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

	// 信号描画
	auto drawSignal = [&](const juce::Colour& color, const std::vector<float>& drawingBuffer) {

		// FFT実施
		auto& real = mDrawingFftRealBuffer;
		auto& image = mDrawingFftImageBuffer;
		auto& window = mDrawingFftWindowCache;
		if(mStrechRate > 1) {
			const size_t window_offset = mUnitBufferSize * (mStrechRate - 1) / 2;
			std::fill(real.begin(), real.end(), 0.f);
			for(size_t i = 0; i < drawingBuffer.size(); ++i) {
				real[window_offset + i] = drawingBuffer[i] * window[i];
			}
		} else {
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
	};
	drawSignal(juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 0.5f), mDrawingBuffer1ch);
	drawSignal(juce::Colour::fromFloatRGBA(0.f, 0.f, 1.f, 0.5f), mDrawingBuffer2ch);

	// 枠描画
	g.setColour(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 1.f));
	g.drawRect(rect);
}