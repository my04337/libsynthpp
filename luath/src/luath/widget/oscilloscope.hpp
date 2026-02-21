#pragma once

#include <luath/core/core.hpp>

namespace luath::widget
{

class OscilloScope
{
public:
	OscilloScope(uint32_t sampleFreq, uint32_t bufferLength);
	~OscilloScope();

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

	// オシロスコープを描画します
	void draw(ID2D1RenderTarget& renderer, float x, float y, float width, float height);
	
private:
	const uint32_t mSampleFreq;
	const uint32_t mBufferLength;

	mutable std::mutex mInputMutex;
	std::deque<float> mInputBuffer1ch; // リングバッファ
	std::deque<float> mInputBuffer2ch; // リングバッファ
	std::vector<float> mDrawingBuffer1ch; // 描画用バッファ。排他不要。
	std::vector<float> mDrawingBuffer2ch; // 描画用バッファ。排他不要。
};


//
}