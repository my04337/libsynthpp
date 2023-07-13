#pragma once

#include <luath-app/core/core.hpp>

namespace luath::drawing
{

class FontLoader
	: non_copy_move
{
public:
	FontLoader(const CComPtr<IDWriteFactory5>& factory = {});
	~FontLoader();

	CComPtr<IDWriteFontCollection1> createFontCollection(const std::wstring& collectionName, const std::filesystem::path& path);
	CComPtr<IDWriteFontCollection1> createFontCollection(const std::wstring& collectionName, const std::vector<std::filesystem::path>& paths);

	[[nodiscard]]
	CComPtr<IDWriteTextFormat> createTextFormat(const std::wstring& collectionName, const std::wstring& fontName, float fontSize);


private:
	std::mutex mMutex;
	CComPtr<IDWriteFactory5> mFactory;
	CComPtr<IDWriteInMemoryFontFileLoader> mInMemoryFontLoader;
	std::unordered_map<
		std::wstring, 
		std::pair<
			CComPtr<IDWriteFontCollection1>, 
			std::list<std::tuple<std::wstring, float, CComPtr<IDWriteTextFormat>>
			>
		>
	> mFontCollections;
};


}