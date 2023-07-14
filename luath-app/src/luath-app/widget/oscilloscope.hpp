#pragma once

#include <luath-app/core/core.hpp>

namespace luath::app::widget
{

class OscilloScope final
	: public juce::Component
{
public:
	OscilloScope();
	~OscilloScope()override;

	// 表示パラメータを指定します
	void setParams(float sampleFreq, float span);

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

	// オシロスコープを描画します
	void paint(juce::Graphics& g)override;
	
private:
	float mSampleFreq;	// [Hz]
	float mSpan;		// [second]
	size_t mBufferSize;

	mutable std::mutex mInputMutex;
	std::deque<float> mInputBuffer1ch; // リングバッファ
	std::deque<float> mInputBuffer2ch; // リングバッファ
	std::vector<float> mDrawingBuffer1ch; // 描画用バッファ。排他不要。
	std::vector<float> mDrawingBuffer2ch; // 描画用バッファ。排他不要。
};


//
}