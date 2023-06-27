#pragma once

#include <luath/core/core.hpp>

namespace luath::widget
{

class Lissajous
{
public:
	Lissajous(uint32_t sampleFreq, uint32_t bufferLength);
	~Lissajous();

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

	// リサージュ曲線を描画を描画します
	void draw(ID2D1RenderTarget& renderer, float x, float y, float width, float height);
	
private:
	const uint32_t mSampleFreq;
	const uint32_t mBufferLength;

	mutable std::mutex mInputMutex;
	std::deque<std::pair<float, float>> mInputBuffer; // リングバッファ
	std::vector<std::pair<float, float>> mDrawingBuffer; // 描画用バッファ。排他不要。
};

//
}