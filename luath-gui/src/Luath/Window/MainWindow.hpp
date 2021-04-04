#pragma once

#include <Luath/Base/Base.hpp>
#include <Luath/Widget/OscilloScope.hpp>
#include <Luath/Widget/SpectrumAnalyzer.hpp>
#include <Luath/Widget/Lissajous.hpp>
#include <LSP/Synth/Luath.hpp>
#include <LSP/MIDI/Sequencer.hpp>
#include <LSP/Threading/EventSignal.hpp>
#include <LSP/Audio/SDLOutput.hpp>

// より低遅延なオーディオ出力APIが使える場合、それを使用する
#ifdef WIN32
#define USE_WASAPI_OUTPUT
#endif

#ifdef USE_WASAPI_OUTPUT
#include <LSP/Audio/WasapiOutput.hpp>
#endif

namespace Luath::Window
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


protected:
	void loadMidi(const std::filesystem::path& path);
	void drawingThreadMain();
	void onRenderedSignal(LSP::Signal<float>&& sig);

private:
	SDL_Window* mWindow = nullptr;

	// 描画スレッド
	std::thread mDrawingThread;
	std::mutex mDrawingMutex;
	std::atomic_bool mDrawingThreadAborted;

	// 再生用ストリーム
#ifdef USE_WASAPI_OUTPUT
	LSP::Audio::WasapiOutput mOutput;
#else
	LSP::Audio::SDLOutput mOutput;
#endif
	// 再生パラメータ
	std::atomic<float> mPostAmpVolume = 1.0f;

	// シーケンサ,シンセサイザ
	LSP::Synth::Luath mSynthesizer;
	LSP::MIDI::Sequencer mSequencer;

	// 各種ウィジット
	Luath::Widget::OscilloScope mOscilloScopeWidget;
	Luath::Widget::SpectrumAnalyzer mSpectrumAnalyzerWidget;
	Luath::Widget::Lissajous mLissajousWidget;
};

}