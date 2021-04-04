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

Text Text::make(SDL_Renderer* renderer, TTF_Font* font, const wchar_t* textW, SDL_Color color)
{
	lsp_assert(renderer != nullptr);
	lsp_assert(font != nullptr);
	lsp_assert(sizeof(wchar_t) == sizeof(Uint16));
	
	auto surface = TTF_RenderUNICODE_Blended(font, reinterpret_cast<const Uint16 *>(textW), color);
	if(!surface) return {};
	auto fin_act_free_surface = finally([&]{SDL_FreeSurface(surface);});

	Text text;
	text.mRenderer = renderer;
	text.mTexture = SDL_CreateTextureFromSurface(renderer, surface);
	text.mWidth = surface->w;
	text.mHeight = surface->h;
	return text; // NRVO
}
void Text::dispose()
{
	if (mTexture) {
		SDL_DestroyTexture(mTexture);
		mTexture = nullptr;
	}
}
void Text::draw(int x, int y)
{
	if (!mRenderer || !mTexture) return;

	SDL_Rect rect{ x, y, mWidth, mHeight };
	SDL_RenderCopy(mRenderer, mTexture, nullptr, &rect);
}