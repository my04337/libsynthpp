/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/lissajous.hpp>

using namespace luath::app;
using namespace luath::app::widget;

Lissajous::Lissajous()
{
	setParams(1.f, 1.f);
}

Lissajous::~Lissajous()
{
}

void Lissajous::setParams(float sampleFreq, float span)
{
	require(sampleFreq > 0);
	require(span > 0);

	auto bufferSize = static_cast<size_t>(sampleFreq * span);
	require(bufferSize > 0);

	std::lock_guard lock(mInputMutex);
	mSampleFreq = sampleFreq;
	mSpan = span;
	mBufferSize = bufferSize;

	mInputBuffer.resize(bufferSize, std::make_pair(0.f, 0.f));
	mDrawingBuffer.resize(bufferSize, std::make_pair(0.f, 0.f));
}

void Lissajous::write(const Signal<float>& sig)
{
	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_samples = sig.samples();

	require(signal_channels == 2, "Lissajous : write - failed (channel count is mismatch)");

	// バッファ末尾に追記
	for(size_t i = 0; i < signal_samples; ++i) {
		mInputBuffer.emplace_back(sig.data(0, i), sig.data(1, i));
	}

	// リングバッファとして振る舞うため、先頭から同じサイズを削除
	mInputBuffer.erase(mInputBuffer.begin(), mInputBuffer.begin() + signal_samples);
}

void Lissajous::paint(juce::Graphics& g, const float left, const float top, const float width, const float height)
{
	// 信号出力をブロックしないように描画用信号バッファへコピー
	{
		std::lock_guard lock(mInputMutex);
		std::copy(mInputBuffer.begin(), mInputBuffer.end(), mDrawingBuffer.begin());
	}

	// 描画開始
	g.saveState();
	auto fin_act_restore_state = finally([&] {g.restoreState(); });

	const juce::Rectangle<float>  rect{ left, top, width, height };
	juce::Path clipPath;
	clipPath.addRectangle(rect);
	g.reduceClipRegion(clipPath);

	// よく使う値を先に計算
	const float right = rect.getRight();
	const float bottom = rect.getBottom();

	const float mid_x = (left + right) / 2;
	const float mid_y = (top + bottom) / 2;

	const size_t buffer_size = mBufferSize;
	const float sample_pitch = width / buffer_size;

	// 罫線描画
	g.setColour(juce::Colour::fromFloatRGBA(0.5f, 1.f, 0.125f, 1.f));
	for(int i = 1; i <= 9; ++i) {
		float x = left + width * 0.1f * i;
		float y = top + height * 0.1f * i;
		g.drawLine(left, y, right, y);
		g.drawLine(x, top, x, bottom);
	}

	// 信号描画
	g.setColour(juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 1.f));
	juce::Path signalPath;
	for(size_t i = 0; i < buffer_size; ++i) {
		auto [ch1, ch2] = mDrawingBuffer[i];
		float x = mid_x + width / 2.0f * normalize(ch1);
		float y = mid_y - height / 2.0f * normalize(ch2);
		if(i == 0) {
			signalPath.startNewSubPath(x, y);
		}
		else [[likely]] {
			signalPath.lineTo(x, y);
		}
	}
	g.strokePath(signalPath, juce::PathStrokeType(1));

	// 枠描画
	g.setColour(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 1.f));
	g.drawRect(rect);
}