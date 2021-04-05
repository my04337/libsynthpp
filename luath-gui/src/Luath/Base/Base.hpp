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
	// 文字列から描画可能なテクスチャに変換します (注意 : 非常に低速です)
	static Text make(SDL_Renderer* renderer, TTF_Font* font, const std::wstring& text, SDL_Color color = SDL_Color{0,0,0,255});

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

// 文字列高速描画ユーティリティクラス
class FastTextRenderer
	: non_copy_move
{
public:
	FastTextRenderer(SDL_Renderer* renderer, TTF_Font* font, SDL_Color color = SDL_Color{ 0,0,0,255 });
	~FastTextRenderer();

	SDL_Rect draw(int x, int y, const std::wstring& text);

private:
	SDL_Renderer* mRenderer;
	TTF_Font* mFont;
	SDL_Color mColor;
	std::unordered_map<std::wstring, Text> mCachedCharacters; // // SDL_ttfのテクスチャ生成が非常に遅いため、一文字単位で描画した文字をキャッシュする
};


}