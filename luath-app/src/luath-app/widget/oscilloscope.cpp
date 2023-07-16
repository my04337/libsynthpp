#include <luath-app/widget/oscilloscope.hpp>

using namespace luath::app::widget;

OscilloScope::OscilloScope()
{
	setParams(1.f, 1.f);
}

OscilloScope::~OscilloScope()
{
}
void OscilloScope::setParams(float sampleFreq, float span)
{
	require(sampleFreq > 0);
	require(span > 0);

	auto bufferSize = static_cast<size_t>(sampleFreq * span);
	require(bufferSize > 0);

	std::lock_guard lock(mInputMutex);
	mSampleFreq = sampleFreq;
	mSpan = span;
	mBufferSize = bufferSize;

	mInputBuffer1ch.resize(mBufferSize, 0.f);
	mInputBuffer2ch.resize(mBufferSize, 0.f);
	mDrawingBuffer1ch.resize(mBufferSize, 0.f);
	mDrawingBuffer2ch.resize(mBufferSize, 0.f);
}

void OscilloScope::write(const Signal<float>& sig)
{
	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_samples = sig.samples();

	require(signal_channels == 2, "OscilloScope : write - failed (channel count is mismatch)");

	// バッファ末尾に追記
	mInputBuffer1ch.insert(mInputBuffer1ch.end(), sig.data(0), sig.data(0) + signal_samples);
	mInputBuffer2ch.insert(mInputBuffer2ch.end(), sig.data(1), sig.data(1) + signal_samples);

	// リングバッファとして振る舞うため、先頭から同じサイズを削除
	mInputBuffer1ch.erase(mInputBuffer1ch.begin(), mInputBuffer1ch.begin() + signal_samples);
	mInputBuffer2ch.erase(mInputBuffer2ch.begin(), mInputBuffer2ch.begin() + signal_samples);
}

void OscilloScope::paint(juce::Graphics& g)
{
	// 信号出力をブロックしないように描画用信号バッファへコピー
	{
		std::lock_guard lock(mInputMutex);
		std::copy(mInputBuffer1ch.begin(), mInputBuffer1ch.end(), mDrawingBuffer1ch.begin());
		std::copy(mInputBuffer2ch.begin(), mInputBuffer2ch.end(), mDrawingBuffer2ch.begin());
	}

	// 描画開始
	g.saveState();
	auto fin_act_restore_state = finally([&] {g.restoreState(); });

	const juce::Rectangle<float>  rect{ static_cast<float>(getX()), static_cast<float>(getY()), static_cast<float>(getWidth()), static_cast<float>(getHeight())};
	if(rect.isEmpty())return;

	juce::Path clipPath;
	clipPath.addRectangle(rect);
	g.reduceClipRegion(clipPath);

	// よく使う値を先に計算
	const float left = rect.getX();
	const float top = rect.getY();
	const float right = rect.getRight();
	const float bottom = rect.getBottom();
	const float width = rect.getWidth();
	const float height = rect.getHeight();

	const float mid_x = (left + right) / 2;
	const float mid_y = (top + bottom) / 2;

	const size_t buffer_size = mBufferSize;
	const float speed_ratio = buffer_size / width;

	auto& interpolated = mInterpolatedSignalBuffer;
	const size_t interpolated_size = static_cast<size_t>(std::ceil(width));
	interpolated.resize(interpolated_size);


	// 罫線描画
	g.setColour(juce::Colour::fromFloatRGBA(0.5f, 1.f, 0.125f, 1.f));
	for (int i = 1; i <= 9; ++i) {
		float x = left + width  * 0.1f * i;
		float y = top  + height * 0.1f * i;
		g.drawLine(left, y, right, y);
		g.drawLine(x, top, x, bottom);
	}

	// 信号描画
	auto drawSignal = [&](const juce::Colour& color, const std::vector<float>& buffer) {
		// そのまま全サンプルを描画すると重いため、描画画素数に応じた値まで間引く
		juce::LinearInterpolator().process(
			speed_ratio,
			buffer.data(),
			interpolated.data(),
			static_cast<int>(interpolated_size),
			static_cast<int>(buffer_size),
			0
		);

		// 複数回のdrawLine呼び出しは重いため、Pathとして一括で描画する
		auto getY = [&](float v) {return mid_y - height / 2.0f * normalize(v); };		
		juce::Path path;		
		path.startNewSubPath(left, getY(interpolated[0]));
		for(size_t i = 1; i < interpolated_size; ++i) {
			path.lineTo(left + static_cast<float>(i), getY(interpolated[i]));
		}
		g.setColour(color);
		g.strokePath(path, juce::PathStrokeType(1));
	};
	drawSignal(juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 0.5f), mDrawingBuffer1ch);
	drawSignal(juce::Colour::fromFloatRGBA(0.f, 0.f, 1.f, 0.5f), mDrawingBuffer2ch);

	// 枠描画
	g.setColour(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 1.f ));
	g.drawRect(rect);
}