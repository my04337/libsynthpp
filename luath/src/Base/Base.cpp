#include <Luath/Base/Base.hpp>


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
	mTexture = d.mTexture;
	mRect = d.mRect;
	d.mTexture = nullptr;
	d.mRect = SDL_Rect{0, 0, 0, 0};
	return *this;
}
Text::~Text()
{
	dispose();
}

Text Text::make(SDL_Renderer* renderer, TTF_Font* font, const wchar_t* textW, SDL_Color color)
{
	lsp_assert(renderer != nullptr);
	lsp_assert(font != nullptr);
	lsp_assert(sizeof(wchar_t) == sizeof(Uint16));
	
	auto surface = TTF_RenderUNICODE_Blended(font, reinterpret_cast<const Uint16 *>(textW), color);
	if(!surface) return {};
	auto fin_act_free_surface = finally([&]{SDL_FreeSurface(surface);});

	Text text;
	text.mTexture = SDL_CreateTextureFromSurface(renderer, surface);
	text.mRect.w = surface->w;
	text.mRect.h = surface->h;
	return text; // NRVO
}
void Text::dispose()
{
	if (mTexture) {
		SDL_DestroyTexture(mTexture);
		mTexture = nullptr;
	}
}