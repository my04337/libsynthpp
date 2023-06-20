#include <luath_gui/app/font_cache.hpp>

using namespace luath_gui;


FontCache::FontCache()
	: mFontRWops(nullptr)
{
	auto font_path = std::filesystem::current_path();
	font_path.append("assets/font/ume-tgo4.ttf");

	mFontRWops = SDL_RWFromFile(font_path.string().c_str(), "rb");
	lsp::Assertion::check(mFontRWops != nullptr, [](auto& o) {o << "font file load failed : " << SDL_GetError(); });
}
FontCache::~FontCache()
{
	for (auto[size, font] : mFontMap) {
		TTF_CloseFont(font);
	}
	mFontMap.clear();
}

TTF_Font* FontCache::get(int ptsize)
{
	std::lock_guard lock(mMutex);

	auto found = mFontMap.find(ptsize);
	if(found != mFontMap.end()) return found->second;

	auto font = TTF_OpenFontRW(mFontRWops, 0, ptsize);
	SDL_RWseek(mFontRWops, 0, RW_SEEK_SET);
	lsp::Assertion::check(font != nullptr, [](auto& o) {o << "font create failed : " << TTF_GetError(); });
	mFontMap.emplace(ptsize, font);
	return font;
}