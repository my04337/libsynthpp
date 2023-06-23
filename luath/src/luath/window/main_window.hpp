#pragma once

#include <luath/base/base.hpp>
#include <luath/widget/oscilloscope.hpp>
#include <luath/widget/spectrum_analyzer.hpp>
#include <luath/widget/lissajous.hpp>
#include <luath/drawing/font_loader.hpp>
#include <lsp/midi/synth/synthesizer.hpp>
#include <lsp/midi/smf/sequencer.hpp>
#include <lsp/io/wasapi_output.hpp>

namespace luath::window
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
	midi::synth::Synthesizer mSynthesizer;
	midi::smf::Sequencer mSequencer;

	// 各種ウィジット
	widget::OscilloScope mOscilloScopeWidget;
	widget::SpectrumAnalyzer mSpectrumAnalyzerWidget;
	widget::Lissajous mLissajousWidget;
};

}