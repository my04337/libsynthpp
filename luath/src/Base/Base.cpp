#include <Luath/Base/Base.hpp>


using namespace LSP;
using namespace Luath;

TexturedText::TexturedText()noexcept
{
}
TexturedText::TexturedText(TexturedText&& d)noexcept
{
	*this = std::move(d);
}
TexturedText& TexturedText::operator=(TexturedText&& d)noexcept
{
	if(&d == this) return *this;

	dispose();
	mTexture = d.mTexture;
	mWidth = d.mWidth;
	mHeight = d.mHeight;
	d.mTexture = nullptr;
	d.mWidth = 0;
	d.mHeight = 0;
	return *this;
}
TexturedText::~TexturedText()
{
	dispose();
}

TexturedText::TexturedText(SDL_Renderer* renderer, TTF_Font* font, const char* textU8, SDL_Color color)
{
	lsp_assert(font != nullptr);
	
	auto surface = TTF_RenderUTF8_Blended(font, textU8, color);
	if(!surface) return;
	auto fin_act_free_surface = finally([&]{SDL_FreeSurface(surface);});

	mTexture = SDL_CreateTextureFromSurface(renderer, surface);
	mWidth = surface->w;
	mHeight = surface->h;
}
void TexturedText::dispose()
{
	if (mTexture) {
		SDL_DestroyTexture(mTexture);
		mTexture = nullptr;
	}
}