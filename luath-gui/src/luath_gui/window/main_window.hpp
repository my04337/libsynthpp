#pragma once

#include <luath_gui/base/base.hpp>
#include <luath_gui/widget/oscilloscope.hpp>
#include <luath_gui/widget/spectrum_analyzer.hpp>
#include <luath_gui/widget/lissajous.hpp>
#include <luath_gui/drawing/font_loader.hpp>
#include <luath/luath.hpp>
#include <lsp/midi/sequencer.hpp>
#include <lsp/io/wasapi_output.hpp>

namespace luath_gui::window
{

class MainWindow final
	: non_copy_move
{
public:
	MainWindow();
	~MainWindow();

	bool initialize();
	void dispose();
	bool onKeyDown(uint16_t key, bool shift, bool ctrl, bool alt);
	void onDropFile(const std::vector<std::filesystem::path>& paths);
	void onDpiChanged(float scale);


protected:
	void loadMidi(const std::filesystem::path& path);
	void onRenderedSignal(lsp::Signal<float>&& sig);
	void onDraw();
	void onDraw(ID2D1RenderTarget& renderer);


private:
	static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:	
	HWND mWindowHandle = nullptr;

	// 描画機構
	struct DrawingContext;
	std::unique_ptr<DrawingContext> mDrawingContext;
	CComPtr<ID2D1Factory> mD2DFactory;
	drawing::FontLoader mFontLoader;

	// 再生用ストリーム
	lsp::io::WasapiOutput mOutput;

	// 再生パラメータ
	std::atomic<float> mPostAmpVolume = 1.0f;

	// シーケンサ,シンセサイザ
	Luath mSynthesizer;
	midi::Sequencer mSequencer;

	// 各種ウィジット
	widget::OscilloScope mOscilloScopeWidget;
	widget::SpectrumAnalyzer mSpectrumAnalyzerWidget;
	widget::Lissajous mLissajousWidget;
};

}