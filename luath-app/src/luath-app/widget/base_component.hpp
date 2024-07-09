// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <luath-app/core/core.hpp>
#include <lsp/util/auto_reset_event.hpp>

namespace luath::app::widget
{

class BaseComponent
	: public juce::Component
{
public:
	using Params = std::unordered_map<std::string, std::any, lsp::string_hash, std::equal_to<>>;

public:
	BaseComponent();
	~BaseComponent() override;

	// コンテンツを描画します
	void paint(juce::Graphics& g)override final;

protected:
	// 描画パラメータを指定します
	void setParam(std::string_view key, std::any&& value);

	// 描画パラメータを削除します
	void unsetParam(std::string_view key);

	// スレッドセーフに再描画を予約します
	void repaintAsync();

	// 描画時にコールバックされます
	virtual void onRendering(juce::Graphics& g, int width, int height, Params& params) = 0;

private:
	// 描画スレッドメイン
	void renderingThreadMain(std::stop_token stopToken);

private:
	// 各種パラメータ ※mInputMutexにて保護される
	// MEMO 描画の度にコピーされるため最小限のデータのみ格納すること
	Params mParams;
	mutable std::mutex mMutexForParams;
	
	// 描画スレッド
	std::jthread mRenderingThread;

	// 描画キャッシュ ※mRenderingMutexにて保護される
	// MEMO juce::Imageの内部にある参照カウント式リソース管理はスレッドセーフではないため、ダブルバッファリングを行う
	mutable std::mutex mRenderingMutex;
	AutoResetEvent mRenderingRequestEvent;
	std::array<juce::Image, 2> mRenderedImages;
	juce::Image* mRenderedImageForPaint = &mRenderedImages[0];
	juce::Image* mRenderedImageForRendering = &mRenderedImages[1];
};

} // namespace luath::app::widget