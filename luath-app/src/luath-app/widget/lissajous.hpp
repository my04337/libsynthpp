/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>

namespace luath::app::widget
{

class Lissajous
{
public:
	Lissajous();
	~Lissajous();

	// 表示パラメータを指定します
	void setParams(float sampleFreq, float span);

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

	// リサージュ曲線を描画を描画します
	void draw(ID2D1RenderTarget& renderer, float x, float y, float width, float height);
	
private:
	float mSampleFreq; // [hz]
	float mSpan;       // [second]
	size_t mBufferSize;

	mutable std::mutex mInputMutex;
	std::deque<std::pair<float, float>> mInputBuffer; // リングバッファ
	std::vector<std::pair<float, float>> mDrawingBuffer; // 描画用バッファ。排他不要。
};

//
}