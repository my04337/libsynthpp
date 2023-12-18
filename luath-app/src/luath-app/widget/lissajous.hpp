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

class Lissajous final
	: public juce::Component
{
public:
	Lissajous();
	~Lissajous();

	// 表示パラメータを指定します
	void setParams(float sampleFreq, float span);

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

	// リサージュ曲線を描画を描画します
	void paint(juce::Graphics& g)override;
	
private:
	void renderThreadMain(std::stop_token stopToken);

private:

	// 各種パラメータ ※mInputMutexにて保護される
	float mSampleFreq; // [hz]

	// 入力用信号バッファ ※mInputMutexにて保護される
	mutable std::mutex mInputMutex;
	std::array<std::deque<float>, 2> mInputBuffer;

	// 描画用パラメータ ※mInputMutexにて保護される
	int mComponentWidthForDrawing = 0;
	int mComponentHeightForDrawing = 0;
	float mScaleFactorForDrawing = 1.f;

	// 描画スレッド
	std::jthread mDrawingThead;

	// 描画キャッシュ ※mDrawingMutexにて保護される
	mutable std::mutex mDrawingMutex;
	AutoResetEvent mRequestDrawEvent;
	juce::Image mDrawnImage;
};

//
}