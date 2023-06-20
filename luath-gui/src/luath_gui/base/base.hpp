#pragma once

#include <lsp/base/base.hpp>
#include <lsp/base/signal.hpp>
#include <lsp/base/logging.hpp>

#include <SDL.h>
#include <SDL_ttf.h>

#include <sstream>
#include <iomanip>

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
		
	float width()const noexcept { return mWidth; }
	float height()const noexcept { return mHeight; }

	void draw(float x, float y);


private:
	SDL_Renderer* mRenderer = nullptr;
	SDL_Texture* mTexture = nullptr;
	float mWidth = 0;
	float mHeight = 0;
};

// 文字列高速描画ユーティリティクラス
class FastTextRenderer
	: non_copy
{
public:
	FastTextRenderer() = default;
	FastTextRenderer(SDL_Renderer* renderer, TTF_Font* font, SDL_Color color = SDL_Color{ 0,0,0,255 });
	~FastTextRenderer() = default;

	FastTextRenderer(FastTextRenderer&&)noexcept = default;
	FastTextRenderer& operator=(FastTextRenderer&&)noexcept = default;

	SDL_FRect draw(float x, float y, const std::wstring& text);

private:
	SDL_Renderer* mRenderer = nullptr;
	TTF_Font* mFont = nullptr;
	SDL_Color mColor = SDL_Color{ 0,0,0,255 };
	std::unordered_map<std::wstring, Text> mCachedCharacters; // // SDL_ttfのテクスチャ生成が非常に遅いため、一文字単位で描画した文字をキャッシュする
};


}