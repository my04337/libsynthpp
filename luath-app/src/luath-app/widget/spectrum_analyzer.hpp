// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/base_component.hpp>
#include <lsp/util/thread_pool.hpp>

namespace luath::app::widget
{

class SpectrumAnalyzer final
	: public BaseComponent
{
public:
	SpectrumAnalyzer();
	~SpectrumAnalyzer()override;

	// 表示パラメータを指定します
	void setParams(float sampleFreq, size_t bufferSize, uint32_t strechRate = 1);

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

protected:
	void onRendering(juce::Graphics& g, int width, int height, Params& params)override;
	
private:
	// 対数軸への変換関数
	static float freq2horz(float width, float freq);
	static float horz2freq(float width, float x);
	static float power2vert(float height, float power);

private:
	// 入力用信号バッファ ※mInputMutexにて保護される
	mutable std::mutex mInputMutex;
	std::array<std::deque<float>, 2> mInputBuffer;

	// FFTウィンドウ形状キャッシュ
	std::vector<float> mDrawingFftWindowShapeCache;

	// FFT並列実行用スレッドプール
	ThreadPool mThreadPoolForFft;
};

//
}