/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <lsp/util/auto_reset_event.hpp>

namespace luath::app::widget
{

class AbstractDrawableComponent
	: public juce::Component
{
public:
	using Params = std::unordered_map<std::string, std::any, lsp::string_hash, std::equal_to<>>;

public:
	AbstractDrawableComponent();
	~AbstractDrawableComponent() override;

	// コンテンツを描画します
	void paint(juce::Graphics& g)override final;

protected:
	// 描画パラメータを指定します
	void setParam(std::string_view key, std::any&& value);

	// 描画パラメータを削除します
	void unsetParam(std::string_view key);

	// 描画時にコールバックされます
	virtual void onDrawElements(juce::Graphics& g, int width, int height, Params& params) = 0;

private:
	// 描画スレッドメイン
	void drawingThreadMain(std::stop_token stopToken);

private:
	// 各種パラメータ ※mInputMutexにて保護される
	// MEMO 描画の度にコピーされるため最小限のデータのみ格納すること
	Params mParams;
	mutable std::mutex mMutexForParams;
	
	// 描画スレッド
	std::jthread mDrawingThead;

	// 描画キャッシュ ※mDrawingMutexにて保護される
	// MEMO juce::Imageの内部にある参照カウント式リソース管理はスレッドセーフではないため、ダブルバッファリングを行う
	mutable std::mutex mDrawingMutex;
	AutoResetEvent mRequestDrawEvent;
	std::array<juce::Image, 2> mDrawnImages;
	juce::Image* mDrawnImageForPaint = &mDrawnImages[0];
	juce::Image* mDrawnImageForDraw = &mDrawnImages[1];
};

} // namespace luath::app::widget