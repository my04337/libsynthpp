#include <luath/widget/oscilloscope.hpp>

using namespace luath;
using namespace luath::widget;

OscilloScope::OscilloScope(uint32_t sampleFreq, uint32_t bufferLength)
	: mSampleFreq(sampleFreq)
	, mBufferLength(bufferLength)
{
	lsp_require(sampleFreq > 0);
	lsp_require(bufferLength > 0);

	mInputBuffer1ch.resize(mBufferLength, 0.f);
	mInputBuffer2ch.resize(mBufferLength, 0.f);
	mDrawingBuffer1ch.resize(mBufferLength, 0.f);
	mDrawingBuffer2ch.resize(mBufferLength, 0.f);
}

OscilloScope::~OscilloScope()
{
}

void OscilloScope::write(const Signal<float>& sig)
{
	std::lock_guard lock(mInputMutex);

	const auto signal_channels = sig.channels();
	const auto signal_frames = sig.frames();

	lsp_require(signal_channels == 2);

	// バッファ末尾に追記
	for(size_t i = 0; i < signal_frames; ++i) {
		auto frame = sig.frame(i);
		mInputBuffer1ch.emplace_back(frame[0]);
		mInputBuffer2ch.emplace_back(frame[1]);
	}

	// リングバッファとして振る舞うため、先頭から同じサイズを削除
	mInputBuffer1ch.erase(mInputBuffer1ch.begin(), mInputBuffer1ch.begin() + signal_frames);
	mInputBuffer2ch.erase(mInputBuffer2ch.begin(), mInputBuffer2ch.begin() + signal_frames);
}

void OscilloScope::draw(ID2D1RenderTarget& renderer, const float left, const float top, const float width, const float height)
{
	// 信号出力をブロックしないように描画用信号バッファへコピー
	{
		std::lock_guard lock(mInputMutex);
		std::copy(mInputBuffer1ch.begin(), mInputBuffer1ch.end(), mDrawingBuffer1ch.begin());
		std::copy(mInputBuffer2ch.begin(), mInputBuffer2ch.end(), mDrawingBuffer2ch.begin());
	}

	// 描画開始
	CComPtr<ID2D1Factory> factory;
	renderer.GetFactory(&factory);
	lsp_check(factory != nullptr);

	CComPtr<ID2D1SolidColorBrush> brush;
	renderer.CreateSolidColorBrush({ 0.f, 0.f, 0.f, 1.f }, &brush);

	// ステータス & クリッピング
	CComPtr<ID2D1DrawingStateBlock> drawingState;
	lsp_check(SUCCEEDED(factory->CreateDrawingStateBlock(&drawingState)));
	renderer.SaveDrawingState(drawingState);

	const D2D1_RECT_F  rect{ left, top, left + width, top + height };
	renderer.PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	auto fin_act= finally([&renderer, &drawingState] {
		renderer.PopAxisAlignedClip(); 
		renderer.RestoreDrawingState(drawingState);
	});

	// よく使う値を先に計算
	const float right  = rect.right;
	const float bottom = rect.bottom;

	const float mid_x = (left + right) / 2;
	const float mid_y = (top + bottom) / 2;

	const uint32_t buffer_length = mBufferLength;
	const float sample_pitch = width / (float)mBufferLength;

	// 罫線描画
	brush->SetColor({0.5f, 1.f, 0.125f, 1.f});
	for (int i = 1; i <= 9; ++i) {
		float x = left + width  * 0.1f * i;
		float y = top  + height * 0.1f * i;
		renderer.DrawLine({ left, y }, { right, y }, brush);
		renderer.DrawLine({ x, top }, { x, bottom }, brush);
	}

	// 信号描画
	auto drawSignal = [&](const D2D1_COLOR_F& color, const std::vector<float>& buffer) {
		brush->SetColor(color);
		auto getPoint = [&](size_t pos) -> D2D1_POINT_2F {
			return { left + pos * sample_pitch , mid_y - height / 2.0f * clamp(buffer[pos]) };
		};
		auto prev = getPoint(0);
		for(uint32_t i = 1; i < buffer_length; ++i) {
			auto pt = getPoint(i);
			if(prev.x != pt.x || prev.y != pt.y) {
				renderer.DrawLine(prev, pt, brush);
			}
			prev = pt;
		}
	};
	drawSignal({ 1.f, 0.f, 0.f, 0.5f }, mDrawingBuffer1ch);
	drawSignal({ 0.f, 0.f, 1.f, 0.5f }, mDrawingBuffer2ch);

	// 枠描画
	brush->SetColor({ 0.f, 0.f, 0.f, 1.f });
	renderer.DrawRectangle(rect, brush);
}