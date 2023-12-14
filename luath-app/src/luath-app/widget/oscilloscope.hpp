#pragma once

#include <luath-app/core/core.hpp>

namespace luath::app::widget
{

class OscilloScope
	: public juce::Component
{
public:
	OscilloScope();
	~OscilloScope();

	// 表示パラメータを指定します
	void setParams(float sampleFreq, float span);

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

	// オシロスコープを描画します
	void paint(juce::Graphics& g);
	
private:
	float mSampleFreq;	// [Hz]
	float mSpan;		// [second]
	size_t mBufferSize;

	// 入力用バッファ
	mutable std::mutex mInputMutex;
	std::array<std::deque<float>, 2> mInputBuffer; 
	
	// 描画用バッファ
	std::array<std::vector<float>, 2> mDrawingBuffer;
	std::vector<float> mInterpolatedSignalBuffer;

	// 静的な表示の描画キャッシュ
	juce::Image mCachedStaticImage;
};


//
}