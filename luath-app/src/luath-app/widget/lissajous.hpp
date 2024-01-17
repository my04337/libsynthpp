/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/base_component.hpp>

namespace luath::app::widget
{

class Lissajous final
	: public BaseComponent
{
public:
	Lissajous();
	~Lissajous();

	// 描画パラメータを指定します
	void setParams(float sampleFreq, float span);

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

protected:
	void onRendering(juce::Graphics& g, int width, int height, Params& params)override;

private:
	// 入力用信号バッファ ※mInputMutexにて保護される
	mutable std::mutex mInputMutex;
	std::array<std::deque<float>, 2> mInputBuffer;
};

//
}