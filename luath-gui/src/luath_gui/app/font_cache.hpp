#pragma once

#include <luath_gui/base/base.hpp>

namespace luath_gui
{
class FontCache final
	: non_copy_move
{
public:
	FontCache();
	~FontCache();

	TTF_Font* get(int ptsize);

private:
	std::mutex mMutex;
	SDL_RWops* mFontRWops;
	std::unordered_map<int, TTF_Font*> mFontMap;
};

}