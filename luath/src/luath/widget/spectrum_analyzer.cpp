#include <luath/widget/spectrum_analyzer.hpp>
#include <lsp/util/fft.hpp>

#include <bitset>
#include <array>

using namespace luath;
using namespace luath::widget;

SpectrumAnalyzer::SpectrumAnalyzer()
{
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
}
void SpectrumAnalyzer::setSignalParams(uint32_t sampleFreq, uint32_t channels, uint32_t bufferLength)
{
	mSampleFreq = sampleFreq;
	mChannels = channels;
	mBufferLength = 1;
	while (true) {
		uint32_t newBufferLength = mBufferLength << 1;
		if (newBufferLength < mBufferLength) break; // オーバーフロー対策
		if (newBufferLength > bufferLength) break; // 元のバッファサイズは超えない
		mBufferLength = newBufferLength;
	}

	lsp::require(channels >= 1);
	lsp::require(mBufferLength >= 1 && mBufferLength <= bufferLength);
	lsp::require(std::bitset<sizeof(bufferLength)*8>(mBufferLength).count() == 1);

	_reset();
}
void SpectrumAnalyzer::_reset()
{
	mBuffers.clear();
	mBuffers.resize(mChannels);

	for (uint32_t ch = 0; ch < mChannels; ++ch) {
		auto& buffer = mBuffers[ch];
		for (uint32_t i = 0; i < mBufferLength; ++i) {
			buffer.emplace_back(0.0f);
		}
	}
}


void SpectrumAnalyzer::draw(ID2D1RenderTarget& renderer, const float left, const float top, const float width, const float height)
{
	std::lock_guard lock(mMutex);

	CComPtr<ID2D1Factory> factory;
	renderer.GetFactory(&factory);
	lsp::check(factory != nullptr);

	CComPtr<ID2D1SolidColorBrush> brush;
	renderer.CreateSolidColorBrush({ 0.f, 0.f, 0.f, 1.f }, &brush);

	// ステータス & クリッピング
	CComPtr<ID2D1DrawingStateBlock> drawingState;
	lsp::check(SUCCEEDED(factory->CreateDrawingStateBlock(&drawingState)));
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


	// グラフ目盛り描画
	brush->SetColor({ 0.5f, 1.f, 0.125f, 1.f });
	for (float digit = 1; digit < 5; ++digit) {
		float base_freq = std::powf(10, digit);
		for (int i = 0; i < 10; ++i) {
			float x = left + freq2horz(base_freq * i);
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
	for (uint32_t ch = 0; ch < mChannels; ++ch) {
		auto& buffer = mBuffers[ch];
		switch (ch) {
		case 0:	brush->SetColor({ 1.f, 0.f, 0.f, 0.5f }); break;
		case 1:	brush->SetColor({ 0.f, 0.f, 1.f, 0.5f }); break;
		default:brush->SetColor({ 0.5f, 0.5f, 0.5f, 0.5f }); break;
		}
		// FFT実施
		std::vector<float> real, image;
		real.resize(buffer.size());
		image.resize(buffer.size(), 0);
		for (size_t i = 0; i < real.size(); ++i) {
			real[i] = buffer[i] * lsp::fft::HammingWf(i / (float)real.size());
		}
		lsp::fft::fft1d<float>(real, image, static_cast<int>(buffer.size()), 0, false);

		// 各点の位置を求める
		D2D1_POINT_2F prev;
		for (uint32_t i = 0; i < buffer_length/2; ++i) { // FFT結果の実軸部分の内、データ後半は折り返し雑音のため使用しない
			float x = left + freq2horz(i* frequency_resolution);
			float p = real[i] * real[i] + image[i] * image[i];
			float y = top + power2vert(p);
			D2D1_POINT_2F pt{ x, y };
			if(i > 0 && (prev.x != pt.x || prev.y != pt.y)) {
				renderer.DrawLine(prev, pt, brush);
			}
			prev = pt;
		}
	}

	// 枠描画
	brush->SetColor({ 0.f, 0.f, 0.f, 1.f });
	renderer.DrawRectangle(rect, brush);
}