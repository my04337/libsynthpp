#pragma once

#include <luath-app/core/core.hpp>

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
	void paint(juce::Graphics& g);
	
private:
	float mSampleFreq; // [hz]
	float mSpan;       // [second]
	size_t mBufferSize;

	// 入力用バッファ
	mutable std::mutex mInputMutex;
	std::deque<std::pair<float, float>> mInputBuffer;

	// 描画用バッファ
	std::vector<std::pair<float, float>> mDrawingBuffer; 

	// 静的な表示の描画キャッシュ
	juce::Image mCachedStaticImage;
};

//
}