#pragma once

#include <Luath/Base/Base.hpp>
#include <Luath/Widget/OscilloScope.hpp>
#include <Luath/Widget/Lissajous.hpp>
#include <LSP/Synth/Luath.hpp>
#include <LSP/MIDI/Sequencer.hpp>
#include <LSP/Threading/EventSignal.hpp>
#include <LSP/Audio/SDLOutput.hpp>

namespace Luath::Window
{

class MainWindow final
	: non_copy_move
{
public:
	MainWindow();
	~MainWindow();

	bool initialize();

protected:
	void drawingThreadMain();
	void onRenderedSignal(LSP::Signal<float>&& sig);

private:
	SDL_Window* mWindow = nullptr;

	// 描画スレッド
	std::thread mDrawingThread;
	std::mutex mDrawingMutex;
	std::atomic_bool mDrawingThreadAborted;

	// 再生用ストリーム
	LSP::Audio::SDLOutput mOutput;

	// シーケンサ,シンセサイザ
	LSP::Synth::Luath mSynthesizer;
	LSP::MIDI::Sequencer mSequencer;

	// 各種ウィジット
	Luath::Widget::OscilloScope mOscilloScopeWidget;
	Luath::Widget::Lissajous mLissajousWidget;
};

}