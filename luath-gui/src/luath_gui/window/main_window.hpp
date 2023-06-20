#pragma once

#include <luath_gui/base/base.hpp>
#include <luath_gui/widget/oscilloscope.hpp>
#include <luath_gui/widget/spectrum_analyzer.hpp>
#include <luath_gui/widget/lissajous.hpp>
#include <lsp/synth/luath.hpp>
#include <lsp/midi/sequencer.hpp>
#include <lsp/base/event_signal.hpp>

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
	void onKeyDown(const SDL_KeyboardEvent& ev);
	void onDropFile(const SDL_DropEvent& ev);
	void onDpiChanged(float scale);


protected:
	void loadMidi(const std::filesystem::path& path);
	void drawingThreadMain();
	void onRenderedSignal(lsp::Signal<float>&& sig);

private:
	SDL_Window* mWindow = nullptr;

	// 描画スケール
	std::atomic<float> mDrawingScale = 1.0f;

	// 描画スレッド
	std::thread mDrawingThread;
	std::mutex mDrawingMutex;
	std::atomic_bool mDrawingThreadAborted;

	// 再生用ストリーム
	lsp::io::WasapiOutput mOutput;

	// 再生パラメータ
	std::atomic<float> mPostAmpVolume = 1.0f;

	// シーケンサ,シンセサイザ
	lsp::synth::Luath mSynthesizer;
	lsp::midi::Sequencer mSequencer;

	// 各種ウィジット
	luath_gui::widget::OscilloScope mOscilloScopeWidget;
	luath_gui::widget::SpectrumAnalyzer mSpectrumAnalyzerWidget;
	luath_gui::widget::Lissajous mLissajousWidget;
};

}