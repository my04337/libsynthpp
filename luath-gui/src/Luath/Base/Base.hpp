﻿#pragma once

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

	operator SDL_Texture*()const noexcept { return mTexture; }
	
	int width()const noexcept { return mRect.w; }
	int height()const noexcept { return mRect.h; }

	const SDL_Rect& rect() const { return mRect; }
	const SDL_Rect& rect(int x, int y) { mRect.x = x; mRect.y = y; return rect(); }

private:
	SDL_Texture* mTexture = nullptr;
	SDL_Rect mRect = { 0, 0, 0, 0 };
};
}