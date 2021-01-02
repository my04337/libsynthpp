#pragma once

#include <Luath/Base/Base.hpp>
#include <Luath/Syntesizer/Generator.hpp>
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

	// シーケンサ
	Luath::Synthesizer::ToneGenerator mToneGenerator;
	LSP::MIDI::Sequencer mSequencer;
};

}