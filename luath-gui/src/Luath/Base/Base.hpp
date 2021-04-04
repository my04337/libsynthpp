#pragma once

#include <LSP/minimal.hpp>

#include <SDL.h>
#include <SDL_ttf.h>

#include <sstream>
#include <iomanip>

#define FORMAT_STRING(...) ((std::wostringstream() << __VA_ARGS__).str())

namespace Luath
{
// いくつかのLSPのクラスは便利なためusingしておく
using clock = LSP::clock;
using non_copy = LSP::non_copy;
using non_copy_move = LSP::non_copy_move;

// 前方宣言
class Application;
class FontCache;


// 文字列描画ユーティリティクラス
class Text 
	: non_copy
{
public:
	static Text make(SDL_Renderer* renderer, TTF_Font* font, const wchar_t* text, SDL_Color color = SDL_Color{0,0,0,255});

	Text()noexcept;
	~Text();

	Text(Text&&)noexcept;
	Text& operator=(Text&&)noexcept;

	void dispose();
		
	int width()const noexcept { return mWidth; }
	int height()const noexcept { return mHeight; }

	void draw(int x, int y);


private:
	SDL_Renderer* mRenderer = nullptr;
	SDL_Texture* mTexture = nullptr;
	int mWidth = 0;
	int mHeight = 0;
};
}