#pragma once

#include <luath-app/core/core.hpp>

namespace luath::app::widget
{

class SpectrumAnalyzer
	: public juce::Component
{
public:
	SpectrumAnalyzer();
	~SpectrumAnalyzer()override;

	// 表示パラメータを指定します
	void setParams(float sampleFreq, size_t bufferSize, uint32_t strechRate = 1);

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);


	// スペクトラム解析結果を描画を描画します
	void paint(juce::Graphics& g)override;

private:
	float mSampleFreq; // [hz]
	float mSpan;       // [second]
	uint32_t mStrechRate;
	size_t mUnitBufferSize;

	
	// 入力用バッファ
	mutable std::mutex mInputMutex;
	std::array<std::deque<float>, 2> mInputBuffer; 
	
	// 描画用バッファ
	std::array<std::vector<float>, 2> mDrawingSignalBuffer;
	std::array<std::vector<float>, 2> mDrawingFftRealBuffer;
	std::array<std::vector<float>, 2> mDrawingFftImageBuffer;
	std::vector<float> mDrawingFftWindowShape;

	// 描画キャッシュ
	juce::Image mCachedStaticImage;
};

//
}