#include <luath_gui/base/base.hpp>


using namespace LSP;
using namespace Luath;

Text::Text()noexcept
{
}
Text::Text(Text&& d)noexcept
{
	*this = std::move(d);
}
Text& Text::operator=(Text&& d)noexcept
{
	if(&d == this) return *this;

	dispose();
	mRenderer = d.mRenderer;
	mTexture = d.mTexture;
	mWidth = d.mWidth;
	mHeight = d.mHeight;
	d.mRenderer = nullptr;
	d.mTexture = nullptr;
	d.mWidth = 0;
	d.mHeight = 0;
	return *this;
}
Text::~Text()
{
	dispose();
}

Text Text::make(SDL_Renderer* renderer, TTF_Font* font, const std::wstring& textW, SDL_Color color)
{
	Assertion::require(renderer != nullptr);
	Assertion::require(font != nullptr);
	Assertion::require(sizeof(wchar_t) == sizeof(Uint16));
	
	auto surface = TTF_RenderUNICODE_Blended(font, reinterpret_cast<const Uint16 *>(textW.c_str()), color);
	if(!surface) return {};
	auto fin_act_free_surface = finally([&]{SDL_FreeSurface(surface);});

	Text text;
	text.mRenderer = renderer;
	text.mTexture = SDL_CreateTextureFromSurface(renderer, surface);
	text.mWidth = static_cast<float>(surface->w);
	text.mHeight = static_cast<float>(surface->h);
	return text; // NRVO
}
void Text::dispose()
{
	if (mTexture) {
		SDL_DestroyTexture(mTexture);
		mTexture = nullptr;
	}
}
void Text::draw(float x, float y)
{
	if (!mRenderer || !mTexture) return;

	SDL_FRect rect{ x, y, mWidth, mHeight };
	SDL_RenderCopyF(mRenderer, mTexture, nullptr, &rect);
}

// ---
FastTextRenderer::FastTextRenderer(SDL_Renderer* renderer, TTF_Font* font, SDL_Color color)
	: mRenderer(renderer)
	, mFont(font)
	, mColor(color)
{
}
SDL_FRect FastTextRenderer::draw(float x, float y, const std::wstring& text)
{
	// 高速化のため、一文字毎に分解して描画する
	// TODO サロゲートペアに対応する
	// TODO 改行に対応する
	if (!mRenderer || !mFont) return { x, y, 0.f, 0.f };

	float curX = x;
	float maxHeight = 0.f;

	for(wchar_t ch_ : text) { 
		std::wstring ch(1, ch_);
		Text* text = nullptr;
		if (auto found = mCachedCharacters.find(ch); found != mCachedCharacters.end()) {
			// 既にキャッシュされている場合、それを用いて描画する
			text = &found->second;
		} else {
			// まだ存在しない場合、生成してキャッシュする
			text = &mCachedCharacters.emplace(ch, Text::make(mRenderer, mFont, ch, mColor)).first->second;
		}

		Assertion::check(text != nullptr);
		text->draw(curX, y);
		curX += text->width();
		maxHeight = std::max(maxHeight, text->height());

	}


	return { x, y, curX - x,  maxHeight };
}