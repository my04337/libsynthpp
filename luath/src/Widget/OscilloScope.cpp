#include <Luath/Widget/OscilloScope.hpp>

using namespace Luath;
using namespace Luath::Widget;

OscilloScope::OscilloScope()
{
}

OscilloScope::~OscilloScope()
{
}
// 表示パラメータを設定します
void OscilloScope::setSize(int width, int height)
{
	std::lock_guard lock(mMutex);
	mWidth = width;
	mHeight = height;

	_reset();
}
void OscilloScope::setParam(uint32_t sampleFreq, uint32_t channels, uint32_t bufferLength)
{
	std::lock_guard lock(mMutex);
	mSampleFreq = sampleFreq;
	mChannels = channels;
	mBufferLength = bufferLength;

	_reset();
}
void OscilloScope::_reset()
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


void OscilloScope::draw(SDL_Renderer* renderer, int left_, int top_)
{
	lsp_assert(renderer != nullptr);

	std::lock_guard lock(mMutex);

	const SDL_Rect rect { left_, top_, mWidth, mHeight };

	// クリッピング
	bool has_clip_rect = SDL_RenderIsClipEnabled(renderer);
	SDL_Rect original_clip_rect;
	if(has_clip_rect) SDL_RenderGetClipRect(renderer, &original_clip_rect);
	auto fin_act_reset_clip_rect = LSP::finally([&]{SDL_RenderSetClipRect(renderer, has_clip_rect ? &original_clip_rect : nullptr);});
	SDL_RenderSetClipRect(renderer, &rect);

	// よく使う値を先に計算
	const int left   = rect.x;
	const int top    = rect.y;
	const int right  = rect.x + rect.w;
	const int bottom = rect.y + rect.h;
	const int width  = mWidth;
	const int height = mHeight;

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
	for (uint32_t ch = 0; ch < mChannels; ++ch) {
		switch (ch) {
		case 0:	SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF); break;
		case 1:	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF); break;
		default:SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 0xFF); break;
		}
		auto& buffer = mBuffers[ch];
		int num = 0;
		for (uint32_t i = 0; i < buffer_length; ++i) {
			int x = left + (int)(i * sample_pitch);
			int y = mid_y + (int)(height/2.0f * buffer[i]);
			SDL_Point pt{x, y};
			if(num > 0 && points[num-1].x == pt.x && points[num-1].y == pt.y) continue;
			points[num++] = SDL_Point{x, y};
		}
		SDL_RenderDrawLines(renderer, &points[0], num);
	}

	// 枠描画
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderDrawRect(renderer, &rect);
}