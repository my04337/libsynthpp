#pragma once

#include <LSP/minimal.hpp>

#include <SDL.h>
#include <SDL_ttf.h>

#include <sstream>
#include <iomanip>

#define FORMAT_STRING(...) dynamic_cast<std::ostringstream&>(std::ostringstream() << __VA_ARGS__).str()

namespace Luath
{
// いくつかのLSPのクラスは便利なためusingしておく
using non_copy = LSP::non_copy;
using non_copy_move = LSP::non_copy_move;

// 前方宣言
class Application;
class FontCache;


// 文字列描画ユーティリティクラス
class TexturedText 
	: non_copy
{
public:
	TexturedText()noexcept;
	TexturedText(SDL_Renderer* renderer, TTF_Font* font, const char* textU8, SDL_Color color);
	~TexturedText();

	TexturedText(TexturedText&&)noexcept;
	TexturedText& operator=(TexturedText&&)noexcept;

	operator SDL_Texture*()const noexcept { return mTexture; }
	
	int width()const noexcept { return mWidth; }
	int height()const noexcept { return mHeight; }

	SDL_Rect rect(int x, int y) { return {x, y, mWidth, mHeight}; }

	void dispose();

private:
	SDL_Texture* mTexture = nullptr;
	int mWidth = 0;
	int mHeight = 0;
};
}