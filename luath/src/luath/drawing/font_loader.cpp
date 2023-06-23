#include <luath/drawing/font_loader.hpp>

#include <fstream>

using namespace lsp;
using namespace luath;
using namespace luath::drawing;

FontLoader::FontLoader(const CComPtr<IDWriteFactory5>& factory)
	: mFactory(factory)
{
	// IDWriteFactoryが供給されなかった場合、このクラスで生成する
	check(SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(decltype(*mFactory)), reinterpret_cast<IUnknown**>(&mFactory))));

	// フォントローダの準備 ※後ほど明示的に解放が必要
	//   see https://deep-verdure.hatenablog.com/entry/2022/07/23/190449
	check(SUCCEEDED(mFactory->CreateInMemoryFontFileLoader(&mInMemoryFontLoader)));
	check(SUCCEEDED(mFactory->RegisterFontFileLoader(mInMemoryFontLoader)));
}

FontLoader::~FontLoader()
{
	// フォントローダーの解放
	if (mFactory && mInMemoryFontLoader) {
		mFactory->UnregisterFontFileLoader(mInMemoryFontLoader);
	}
}

CComPtr<IDWriteFontCollection1> FontLoader::createFontCollection(const std::wstring& collectionName, const std::filesystem::path& path)
{
	return createFontCollection(collectionName, std::vector<std::filesystem::path>{path});
}

CComPtr<IDWriteFontCollection1> FontLoader::createFontCollection(const std::wstring& collectionName, const std::vector<std::filesystem::path>& paths)
{
	std::lock_guard lock(mMutex);

	// フォントセットの作成
	CComPtr<IDWriteFontSetBuilder1> fontSetBuilder;
	check(SUCCEEDED(mFactory->CreateFontSetBuilder(&fontSetBuilder)));
	for (auto& path : paths) {
		// フォントデータをメモリ上に展開
		std::ifstream ifs(path, std::ios_base::in | std::ios_base::binary);
		std::vector<char> fontData((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		ifs.close();

		// メモリ上のデータからDirectWrite用のフォント情報を生成する
		// MEMO 少々無駄が多いが、管理が簡単になるためロードしたフォントデータをさらにコピーして保持させている ※CreateInMemoryFontFileReference第四引数参照
		CComPtr<IDWriteFontFile> fontFile;
		check(SUCCEEDED(mInMemoryFontLoader->CreateInMemoryFontFileReference(mFactory, fontData.data(), static_cast<UINT32>(fontData.size()), nullptr, &fontFile)));
		fontData.clear();

		// ロード完了
		check(SUCCEEDED(fontSetBuilder->AddFontFile(fontFile)));
	}
	CComPtr<IDWriteFontSet> fontSet;
	check(SUCCEEDED(fontSetBuilder->CreateFontSet(&fontSet)));

	// フォントコレクションの作成 ※主にIDWriteFontCollection1の第二引数に指定される
	CComPtr<IDWriteFontCollection1> fontCollection;
	check(SUCCEEDED(mFactory->CreateFontCollectionFromFontSet(fontSet, &fontCollection)));
	mFontCollections.insert_or_assign(collectionName, std::make_pair(fontCollection, decltype(mFontCollections)::mapped_type::second_type{}));
	return fontCollection;
}

CComPtr<IDWriteTextFormat> FontLoader::createTextFormat(const std::wstring& collectionName, const std::wstring& fontName, float fontSize)
{
	std::lock_guard lock(mMutex);

	// フォントコレクションの解決
	auto foundCollection = mFontCollections.find(collectionName);
	require(foundCollection != mFontCollections.end());
	auto& [fontCollection, textFormats] = foundCollection->second;

	// 既に定義済のテキストフォーマットがあればそれを返す
	auto foundTextFormat = std::find_if(textFormats.begin(), textFormats.end(), [&](const auto& tp) {
		if (fontSize != std::get<1>(tp)) return false;
		if (fontName != std::get<0>(tp)) return false;
		return true;
	});
	if (foundTextFormat != textFormats.end()) return std::get<CComPtr<IDWriteTextFormat>>(*foundTextFormat);

	// 無ければ生成して返す
	CComPtr<IDWriteTextFormat> newTextFormat;
	check(SUCCEEDED(mFactory->CreateTextFormat(
		fontName.c_str(),
		fontCollection,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		L"", //locale
		&newTextFormat
	)));
	textFormats.emplace_back(std::make_tuple(fontName, fontSize, newTextFormat));
	return newTextFormat;
}