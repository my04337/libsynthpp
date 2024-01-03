/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/abstract_drawable_component.hpp>

namespace luath::app::widget
{

class AbstractSignalComponent
	: public AbstractDrawableComponent
{
public:
	AbstractSignalComponent();
	~AbstractSignalComponent() override;

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

protected:
	// バッファサイズを指定します
	void setSpan(float sampleFreq, float span);
	void setBufferSize(float sampleFreq, size_t bufferSize);

	// 描画時にコールバックされます
	void onDrawElements(juce::Graphics& g, int width, int height, Params& params)override final;

	// 描画サイズ以外に影響の受けない部分の描画時にコールバックされます
	virtual void onDrawStaticElements(juce::Graphics& g, int width, int height, Params& params) = 0;

	// 動的に変化する部分の描画時にコールバックされます
	virtual void onDrawDynamicElements(juce::Graphics& g, int width, int height, Params& params, std::array<std::vector<float>, 2>& buffer) = 0;

private:
	// 入力用信号バッファ ※mInputMutexにて保護される
	mutable std::mutex mInputMutex;
	std::array<std::deque<float>, 2> mInputBuffer;	

	// 描画用信号バッファ ※描画スレッドから飲みアクセスかのう
	juce::Image mCachedStaticImage;
};

} // namespace luath::app::widget