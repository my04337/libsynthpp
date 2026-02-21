#include <luath/widget/spectrum_analyzer.hpp>
#include <lsp/dsp/fft.hpp>

#include <bitset>
#include <array>

using namespace luath;
using namespace luath::widget;

SpectrumAnalyzer::SpectrumAnalyzer(uint32_t sampleFreq, uint32_t bufferLength)
	: mSampleFreq(sampleFreq)
	, mBufferLength(bufferLength)
{
	lsp_require(sampleFreq > 0);
	lsp_require(bufferLength > 0);

	mInputBuffer1ch.resize(mBufferLength, 0.f);
	mInputBuffer2ch.resize(mBufferLength, 0.f);
	mDrawingBuffer1ch.resize(mBufferLength, 0.f);
	mDrawingBuffer2ch.resize(mBufferLength, 0.f);

	mDrawingFftRealBuffer.resize(mBufferLength, 0.f);
	mDrawingFftImageBuffer.resize(mBufferLength, 0.f);
	mDrawingFftWindowCache.resize(mBufferLength);
	for(size_t i = 0; i < bufferLength; ++i) {
		mDrawingFftWindowCache[i] = lsp::dsp::fft::HammingWf(i / (float)mBufferLength);
	}
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
}

void SpectrumAnalyzer::write(const Signal<float>& sig)
{
	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_frames = sig.frames();

	lsp_require(signal_channels == 2);

	// バッファ末尾に追記
	for(size_t i = 0; i < signal_frames; ++i) {
		auto frame = sig.frame(i);
		mInputBuffer1ch.emplace_back(frame[0]);
		mInputBuffer2ch.emplace_back(frame[1]);
	}

	// リングバッファとして振る舞うため、先頭から同じサイズを削除
	mInputBuffer1ch.erase(mInputBuffer1ch.begin(), mInputBuffer1ch.begin() + signal_frames);
	mInputBuffer2ch.erase(mInputBuffer2ch.begin(), mInputBuffer2ch.begin() + signal_frames);
}

void SpectrumAnalyzer::draw(ID2D1RenderTarget& renderer, const float left, const float top, const float width, const float height)
{
	// 信号出力をブロックしないように描画用信号バッファへコピー
	{
		std::lock_guard lock(mInputMutex);
		std::copy(mInputBuffer1ch.begin(), mInputBuffer1ch.end(), mDrawingBuffer1ch.begin());
		std::copy(mInputBuffer2ch.begin(), mInputBuffer2ch.end(), mDrawingBuffer2ch.begin());
	}

	// 描画開始
	CComPtr<ID2D1Factory> factory;
	renderer.GetFactory(&factory);
	lsp_check(factory != nullptr);

	CComPtr<ID2D1SolidColorBrush> brush;
	renderer.CreateSolidColorBrush({ 0.f, 0.f, 0.f, 1.f }, &brush);

	// ステータス & クリッピング
	CComPtr<ID2D1DrawingStateBlock> drawingState;
	lsp_check(SUCCEEDED(factory->CreateDrawingStateBlock(&drawingState)));
	renderer.SaveDrawingState(drawingState);

	const D2D1_RECT_F  rect{ left, top, left + width, top + height };
	renderer.PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	auto fin_act = lsp::finally([&renderer, &drawingState] {
		renderer.PopAxisAlignedClip();
		renderer.RestoreDrawingState(drawingState);
	});

	// よく使う値を先に計算
	static const float log_min_freq = std::log10f(20); // Hz
	static const float log_max_freq = std::log10f(20000); // Hz
	static const float log_min_dbfs = -20; // dbFS(power)
	static const float log_max_dbfs = +60; // dbFS(power)
	static const float horizontal_resolution = 0.5f; // px ※高周波部分の描画間引用

	const float right = rect.right;
	const float bottom = rect.bottom;

	const float mid_x = (left + right) / 2;
	const float mid_y = (top + bottom) / 2;

	const uint32_t buffer_length = mBufferLength;
	const float frequency_resolution = static_cast<float>(mSampleFreq) / static_cast<float>(mBufferLength); // 周波数分解能


	// 対数軸への変換関数
	auto freq2horz = [&](float freq)->float {
		if (freq < 1.0e-8f) freq = 1.0e-8f; // オーバーフロー対策の補正
		float f = std::log10(freq);

		return std::clamp((f - log_min_freq) / (log_max_freq - log_min_freq) * width, 0.f, width - 1.f);
	};
	auto power2vert = [&](float power)->float {
		if (power < 1.0e-8f) power = 1.0e-8f; // オーバーフロー対策の補正
		float dbfs = 10*std::log10f(power);

		return std::clamp(height - (dbfs - log_min_dbfs) / (log_max_dbfs - log_min_dbfs) * height, 0.f, height - 1.f);
	};
	auto horz2freq = [&](float x)->float {
		// freq2horzの逆変換を行う
		return powf(10.f, x / width * (log_max_freq - log_min_freq) + log_min_freq);
	};


	// グラフ目盛り描画
	brush->SetColor({ 0.5f, 1.f, 0.125f, 1.f });
	for (float digit = 1; digit < 5; ++digit) {
		float base_freq = std::powf(10, digit);
		for (int i = 0; i < 10; ++i) {
			float x = left + freq2horz(base_freq * i);
			float ff = horz2freq(x - left);
			renderer.DrawLine({ x, top }, { x, bottom - 1.f }, brush);
		}
	}
	for (float digit = log_min_dbfs/10; digit < log_max_dbfs/10; ++digit) {
		float base_v = std::powf(10, digit);
		for (int i = 0; i < 10; ++i) {
			float y = top + power2vert(base_v * i);
			renderer.DrawLine({ left, y }, { right - 1.f, y }, brush);
		}
	}

	// 信号描画
	auto drawSignal = [&](const D2D1_COLOR_F& color, const std::vector<float>& buffer) {
		brush->SetColor(color);

		// FFT実施
		auto& real = mDrawingFftRealBuffer;
		auto& image = mDrawingFftImageBuffer;
		auto& window = mDrawingFftWindowCache;
		for(size_t i = 0; i < real.size(); ++i) {
			real[i] = buffer[i] * window[i];
		}
		std::fill(image.begin(), image.end(), 0.f);
		lsp::dsp::fft::fft1d<float>(real.data(), image.data(), static_cast<int>(buffer.size()), 0, false);

		// 各点の位置を求める
		auto getPoint = [&](size_t pos) -> D2D1_POINT_2F {
			float x = left + freq2horz(pos * frequency_resolution);
			float power = real[pos] * real[pos] + image[pos] * image[pos];
			float y = top + power2vert(power);
			return { x, y };
		};
		auto prev = getPoint(0);
		float yPeak = bottom;
		for(uint32_t i = 1; i < buffer_length / 2; ++i) { // FFT結果の実軸部分の内、データ後半は折り返し雑音のため使用しない
			auto pt = getPoint(i);
			yPeak = std::min(yPeak, pt.y);
			if(pt.x - prev.x >= horizontal_resolution) {
				pt.y = yPeak;
				renderer.DrawLine(prev, pt, brush);
				yPeak = bottom;
				prev = pt;
			}
		}
	};
	drawSignal({ 1.f, 0.f, 0.f, 0.5f }, mDrawingBuffer1ch);
	drawSignal({ 0.f, 0.f, 1.f, 0.5f }, mDrawingBuffer2ch);

	// 枠描画
	brush->SetColor({ 0.f, 0.f, 0.f, 1.f });
	renderer.DrawRectangle(rect, brush);
}