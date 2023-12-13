#include <luath-app/widget/lissajous.hpp>

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

void Lissajous::paint(juce::Graphics& g)
{
	// 信号出力をブロックしないように描画用信号バッファへコピー
	{
		std::lock_guard lock(mInputMutex);
		std::copy(mInputBuffer.begin(), mInputBuffer.end(), mDrawingBuffer.begin());
	}

	// 描画サイズが0ならなにもしない
	if(getWidth() <= 0 || getHeight() <= 0) return;

	// 描画領域の算出
	

	// 静的部分の描画開始
	if(mCachedStaticImage.getWidth() != getWidth() || mCachedStaticImage.getHeight() != getHeight()) {
		mCachedStaticImage = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
		juce::Graphics g(mCachedStaticImage);

		const juce::Rectangle<float>  rect{ 0.f, 0.f, static_cast<float>(getWidth()), static_cast<float>(getHeight()) };

		const float left = rect.getX();
		const float top = rect.getY();
		const float right = rect.getRight();
		const float bottom = rect.getBottom();
		const float width = rect.getWidth();
		const float height = rect.getHeight();

		// 罫線描画
		g.setColour(juce::Colour::fromFloatRGBA(0.5f, 1.f, 0.125f, 1.f));
		for(int i = 1; i <= 9; ++i) {
			float x = left + width * 0.1f * i;
			float y = top + height * 0.1f * i;
			g.drawLine(left, y, right, y);
			g.drawLine(x, top, x, bottom);
		}

		// 枠描画
		g.setColour(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 1.f));
		g.drawRect(rect);
	}

	// 動的部分の描画開始
	{
		const juce::Rectangle<float>  rect{ static_cast<float>(getX()), static_cast<float>(getY()), static_cast<float>(getWidth()), static_cast<float>(getHeight()) };

		const float left = rect.getX();
		const float top = rect.getY();
		const float right = rect.getRight();
		const float bottom = rect.getBottom();
		const float width = rect.getWidth();
		const float height = rect.getHeight();

		const float mid_x = (left + right) / 2;
		const float mid_y = (top + bottom) / 2;
		const size_t buffer_size = mBufferSize;
		const float sample_pitch = width / buffer_size;

		g.saveState();
		auto fin_act_restore_state = finally([&] {g.restoreState(); });


		// 描画済の静的部分を転写
		g.drawImageAt(mCachedStaticImage, left, top);

		// 枠の内側に描画されるようにクリッピング
		juce::Path clipPath;
		auto clipRect = rect.expanded(-1.f);
		clipPath.addRectangle(clipRect);
		g.reduceClipRegion(clipPath);


		// 信号描画
		auto getPoint = [&](size_t pos) -> juce::Point<float> {
			auto [ch1, ch2] = mDrawingBuffer[pos];
			return {
				mid_x + width / 2.0f * normalize(ch1),
				mid_y - height / 2.0f * normalize(ch2),
			};
			};
		juce::Path signalPath;
		signalPath.startNewSubPath(getPoint(0));
		for(size_t i = 1; i < buffer_size; ++i) {
			auto cur = getPoint(i);
			if(cur.getDistanceFrom(signalPath.getCurrentPosition()) >= 1.f) {
				signalPath.lineTo(cur);
			}
		}
		g.setColour(juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 1.f));
		g.strokePath(signalPath, juce::PathStrokeType(1));

	}
}