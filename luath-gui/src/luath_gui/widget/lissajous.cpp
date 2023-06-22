#include <luath_gui/widget/lissajous.hpp>

using namespace luath_gui;
using namespace luath_gui::widget;

Lissajous::Lissajous()
{
}

Lissajous::~Lissajous()
{
}
void Lissajous::setParam(uint32_t sampleFreq, uint32_t channels, uint32_t bufferLength)
{
	mSampleFreq = sampleFreq;
	mChannels = channels;
	mBufferLength = bufferLength;

	lsp::require(channels >= 2);

	_reset();
}
void Lissajous::_reset()
{
	mBuffers.clear();
	mBuffers.resize(mChannels);

	for (uint32_t ch = 0; ch < mChannels; ++ch) {
		auto& buffer = mBuffers[ch];
		for (uint32_t i = 0; i < mBufferLength; ++i) {
			buffer.emplace_back(0.0f);
		}
	}
}


void Lissajous::draw(ID2D1RenderTarget& renderer, const float left, const float top, const float width, const float height)
{
	std::lock_guard lock(mMutex);

	CComPtr<ID2D1Factory> factory;
	renderer.GetFactory(&factory);
	lsp::check(factory != nullptr);

	CComPtr<ID2D1SolidColorBrush> brush;
	renderer.CreateSolidColorBrush({ 0.f, 0.f, 0.f, 1.f }, &brush);

	// ステータス & クリッピング
	CComPtr<ID2D1DrawingStateBlock> drawingState;
	lsp::check(SUCCEEDED(factory->CreateDrawingStateBlock(&drawingState)));
	renderer.SaveDrawingState(drawingState);

	const D2D1_RECT_F  rect{ left, top, left + width, top + height };
	renderer.PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	auto fin_act = lsp::finally([&renderer, &drawingState] {
		renderer.PopAxisAlignedClip();
		renderer.RestoreDrawingState(drawingState);
	});

	// よく使う値を先に計算
	const float right = rect.right;
	const float bottom = rect.bottom;

	const float mid_x = (left + right) / 2;
	const float mid_y = (top + bottom) / 2;

	const uint32_t buffer_length = mBufferLength;
	const float sample_pitch = width / (float)mBufferLength;

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
	for (uint32_t i = 0; i < buffer_length; ++i) {
		float x = mid_x + width / 2.0f * lsp::normalize(mBuffers[0][i]);
		float y = mid_y - height / 2.0f * lsp::normalize(mBuffers[1][i]);
		D2D1_POINT_2F pt{x, y};
		if(i > 0 && (prev.x != pt.x || prev.y != pt.y)) {
			renderer.DrawLine(prev, pt, brush);
		}
		prev = pt;
	}

	// 枠描画
	brush->SetColor({ 0.f, 0.f, 0.f, 1.f });
	renderer.DrawRectangle(rect, brush);
}