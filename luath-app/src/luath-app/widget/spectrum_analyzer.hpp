/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/abstract_signal_component.hpp>
#include <lsp/util/thread_pool.hpp>

namespace luath::app::widget
{

class SpectrumAnalyzer final
	: public AbstractSignalComponent
{
public:
	SpectrumAnalyzer();
	~SpectrumAnalyzer()override;

	// 表示パラメータを指定します
	void setParams(float sampleFreq, size_t bufferSize, uint32_t strechRate = 1);

protected:
	void onDrawStaticElements(juce::Graphics& g, int width, int height, Extras& extras)override;
	void onDrawDynamicElements(juce::Graphics& g, int width, int height, Extras& extras, std::array<std::vector<float>, 2>& buffer)override;

private:
	// 対数軸への変換関数
	static float freq2horz(float width, float freq);
	static float horz2freq(float width, float x);
	static float power2vert(float height, float power);

private:
	// FFTウィンドウ形状キャッシュ
	std::vector<float> mDrawingFftWindowShapeCache;

	// FFT並列実行用スレッドプール
	ThreadPool mThreadPoolForFft;
};

//
}