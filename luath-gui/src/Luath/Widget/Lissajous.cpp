#include <Luath/Widget/Lissajous.hpp>

using namespace Luath;
using namespace Luath::Widget;

Lissajous::Lissajous()
{
}

Lissajous::~Lissajous()
{
}
void Lissajous::setParam(uint32_t sampleFreq, uint32_t channels, uint32_t bufferLength)
{
	std::lock_guard lock(mMutex);
	mSampleFreq = sampleFreq;
	mChannels = channels;
	mBufferLength = bufferLength;

	lsp_assert(channels >= 2);

	_reset();
}
void Lissajous::_reset()
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


void Lissajous::draw(SDL_Renderer* renderer, int left_, int top_, int width_, int height_)
{
	lsp_assert(renderer != nullptr);

	std::lock_guard lock(mMutex);

	const SDL_Rect rect { left_, top_, width_, height_};

	// クリッピング	
	SDL_bool is_clipped = SDL_RenderIsClipEnabled(renderer);
	SDL_Rect original_clip_rect;
	if(is_clipped) SDL_RenderGetClipRect(renderer, &original_clip_rect);
	auto fin_act = LSP::finally([&] { SDL_RenderSetClipRect(renderer, is_clipped ? &original_clip_rect : nullptr); });
	SDL_RenderSetClipRect(renderer, &rect);
	
	// よく使う値を先に計算
	const int left   = rect.x;
	const int top    = rect.y;
	const int right  = rect.x + rect.w;
	const int bottom = rect.y + rect.h;
	const int width  = rect.w;
	const int height = rect.h;

	const int mid_x = (left + right) / 2;
	const int mid_y = (top + bottom) / 2;

	const uint32_t buffer_length = mBufferLength;
	const float sample_pitch = width / (float)mBufferLength;


	// 罫線描画
	SDL_SetRenderDrawColor(renderer, 0x80, 0xFF, 0x20, 255);
	for (int i = 1; i <= 9; ++i) {
		int x = left + int(width  * 0.1f * i);
		int y = top  + int(height * 0.1f * i);
		SDL_RenderDrawLine(renderer, left, y, right, y);
		SDL_RenderDrawLine(renderer, x, top, x, bottom);
	}

	// 信号描画
	std::vector<SDL_Point> points(buffer_length);
	SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
	int num = 0;
	for (uint32_t i = 0; i < buffer_length; ++i) {
		int x = mid_x + (int)(width / 2.0f * std::clamp(mBuffers[0][i], LSP::sample_traits<float>::normalized_min, LSP::sample_traits<float>::normalized_max));
		int y = mid_y - (int)(height / 2.0f * std::clamp(mBuffers[1][i], LSP::sample_traits<float>::normalized_min, LSP::sample_traits<float>::normalized_max));
		SDL_Point pt{x, y};
		if(num > 0 && points[num-1].x == pt.x && points[num-1].y == pt.y) continue;
		points[num++] = SDL_Point{x, y};
	}
	SDL_RenderDrawLines(renderer, &points[0], num);

	// 枠描画
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderDrawRect(renderer, &rect);

	// クリッピング解除
	fin_act.action();
	lsp_assert(SDL_RenderIsClipEnabled(renderer) == is_clipped);
}