﻿#include <luath/App/FontCache.hpp>

using namespace Luath;


FontCache::FontCache()
	: mFontRWops(nullptr)
{
	auto font_path = std::filesystem::current_path();
	font_path.append("assets/font/ume-tgo4.ttf");

	mFontRWops = SDL_RWFromFile(font_path.string().c_str(), "rb");
	lsp_assert_desc(mFontRWops != nullptr, "font file load failed : " << SDL_GetError());
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
	lsp_assert_desc(font != nullptr, "font create failed : " << TTF_GetError());
	mFontMap.emplace(ptsize, font);
	return font;
}