#include <luath_gui/widget/spectrum_analyzer.hpp>
#include <lsp/Util/fft.hpp>

#include <bitset>
#include <array>

using namespace Luath;
using namespace Luath::Widget;

SpectrumAnalyzer::SpectrumAnalyzer()
{
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
}
void SpectrumAnalyzer::setParam(uint32_t sampleFreq, uint32_t channels, uint32_t bufferLength)
{
	std::lock_guard lock(mMutex);
	mSampleFreq = sampleFreq;
	mChannels = channels;
	mBufferLength = 1;
	while (true) {
		uint32_t newBufferLength = mBufferLength << 1;
		if (newBufferLength < mBufferLength) break; // オーバーフロー対策
		if (newBufferLength > bufferLength) break; // 元のバッファサイズは超えない
		mBufferLength = newBufferLength;
	}

	LSP::Assertion::require(channels >= 1);
	LSP::Assertion::require(mBufferLength >= 1 && mBufferLength <= bufferLength);
	LSP::Assertion::require(std::bitset<sizeof(bufferLength)*8>(mBufferLength).count() == 1);

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


void SpectrumAnalyzer::draw(SDL_Renderer* renderer, int left_, int top_, int width_, int height_)
{
	LSP::Assertion::require(renderer != nullptr);

	std::lock_guard lock(mMutex);

	const SDL_Rect rect{ left_, top_, width_, height_ };

	// よく使う値を先に計算
	static const float log_min_freq = std::log10f(20); // Hz
	static const float log_max_freq = std::log10f(20000); // Hz
	static const float log_min_dbfs = -20; // dbFS(power)
	static const float log_max_dbfs = +60; // dbFS(power)
	const int left = rect.x;
	const int top = rect.y;
	const int right = rect.x + rect.w;
	const int bottom = rect.y + rect.h;
	const int width = rect.w;
	const int height = rect.h;

	const int mid_x = (left + right) / 2;
	const int mid_y = (top + bottom) / 2;

	const uint32_t buffer_length = mBufferLength;
	const float frequency_resolution = static_cast<float>(mSampleFreq) / static_cast<float>(mBufferLength); // 周波数分解能


	// 対数軸への変換関数
	auto freq2horz = [&](float freq)->int {
		if (freq < 1.0e-8f) freq = 1.0e-8f; // オーバーフロー対策の補正
		float f = std::log10(freq);

		return std::clamp<int>(static_cast<int>((f - log_min_freq) / (log_max_freq - log_min_freq) * width), 0, width-1);
	};
	auto power2vert = [&](float power)->int {
		if (power < 1.0e-8f) power = 1.0e-8f; // オーバーフロー対策の補正
		float dbfs = 10*std::log10f(power);

		return std::clamp<int>(static_cast<int>(height - (dbfs - log_min_dbfs) / (log_max_dbfs - log_min_dbfs) * height), 0, height-1);
	};


	// グラフ目盛り描画
	SDL_SetRenderDrawColor(renderer, 0x80, 0xFF, 0x20, 255);
	for (float digit = 1; digit < 5; ++digit) {
		float base_freq = std::powf(10, digit);
		for (int i = 0; i < 10; ++i) {
			int x = left + freq2horz(base_freq * i);
			SDL_RenderDrawLine(renderer, x, top, x, bottom-1);
		}
	}
	for (float digit = log_min_dbfs/10; digit < log_max_dbfs/10; ++digit) {
		float base_v = std::powf(10, digit);
		for (int i = 0; i < 10; ++i) {
			int y = top + power2vert(base_v * i);
			SDL_RenderDrawLine(renderer, left, y, right-1, y);
		}
	}

	// 信号描画
	std::vector<SDL_Point> points(buffer_length);
	for (uint32_t ch = 0; ch < mChannels; ++ch) {
		auto& buffer = mBuffers[ch];
		switch (ch) {
		case 0:	SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF); break;
		case 1:	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF); break;
		default:SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xFF); break;
		}
		// FFT実施
		std::vector<float> real(buffer.size()), image(buffer.size());
		for (size_t i = 0; i < real.size(); ++i) {
			real[i] = buffer[i] * LSP::Util::FFT::HammingWf(i / (float)real.size());
		}
		LSP::Util::FFT::fft1d<float>(real, image, static_cast<int>(buffer.size()), 0, false);

		// 各点の位置を求める
		int num = 0;
		for (uint32_t i = 0; i < buffer_length/2; ++i) { // FFT結果の実軸部分の内、データ後半は折り返し雑音のため使用しない
			int x = left + freq2horz(i* frequency_resolution);
			float p = real[i] * real[i] + image[i] * image[i];
			int y = top + power2vert(p);
			SDL_Point pt{ x, y };
			if (num > 0 && points[num - 1].x == pt.x && points[num - 1].y == pt.y) continue;
			points[num++] = SDL_Point{ x, y };
		}
		SDL_RenderDrawLines(renderer, &points[0], num);
	}

	// 枠描画
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderDrawRect(renderer, &rect);
}