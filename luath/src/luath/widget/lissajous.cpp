#include <luath/widget/lissajous.hpp>

using namespace luath;
using namespace luath::widget;

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

void Lissajous::draw(ID2D1RenderTarget& renderer, const float left, const float top, const float width, const float height)
{
	// 信号出力をブロックしないように描画用信号バッファへコピー
	{
		std::lock_guard lock(mInputMutex);
		std::copy(mInputBuffer.begin(), mInputBuffer.end(), mDrawingBuffer.begin());
	}

	// 描画開始
	CComPtr<ID2D1Factory> factory;
	renderer.GetFactory(&factory);
	check(factory != nullptr);

	CComPtr<ID2D1SolidColorBrush> brush;
	renderer.CreateSolidColorBrush({ 0.f, 0.f, 0.f, 1.f }, &brush);

	// ステータス & クリッピング
	CComPtr<ID2D1DrawingStateBlock> drawingState;
	check(SUCCEEDED(factory->CreateDrawingStateBlock(&drawingState)));
	renderer.SaveDrawingState(drawingState);

	const D2D1_RECT_F  rect{ left, top, left + width, top + height };
	renderer.PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	auto fin_act = finally([&renderer, &drawingState] {
		renderer.PopAxisAlignedClip();
		renderer.RestoreDrawingState(drawingState);
	});

	// よく使う値を先に計算
	const float right = rect.right;
	const float bottom = rect.bottom;

	const float mid_x = (left + right) / 2;
	const float mid_y = (top + bottom) / 2;

	const size_t buffer_size = mBufferSize;
	const float sample_pitch = width / buffer_size;

	// 罫線描画
	brush->SetColor({ 0.5f, 1.f, 0.125f, 1.f });
	for (int i = 1; i <= 9; ++i) {
		float x = left + width  * 0.1f * i;
		float y = top  + height * 0.1f * i;
		renderer.DrawLine({ left, y }, { right, y }, brush);
		renderer.DrawLine({ x, top }, { x, bottom }, brush);
	}

	// 信号描画
	brush->SetColor({ 1.f, 0.f, 0.f, 1.f });
	D2D1_POINT_2F prev;
	for(size_t i = 0; i < buffer_size; ++i) {
		auto [ch1, ch2] = mDrawingBuffer[i];
		float x = mid_x + width / 2.0f * normalize(ch1);
		float y = mid_y - height / 2.0f * normalize(ch2);
		D2D1_POINT_2F pt{ x, y };
		if(i > 0 && (prev.x != pt.x || prev.y != pt.y)) {
			renderer.DrawLine(prev, pt, brush);
		}
		prev = pt;
	}

	// 枠描画
	brush->SetColor({ 0.f, 0.f, 0.f, 1.f });
	renderer.DrawRectangle(rect, brush);
}