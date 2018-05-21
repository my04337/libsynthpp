#pragma once

#include <Luath/Base/Base.hpp>
#include <LSP/MIDI/SMF/Sequencer.hpp>
#include <LSP/MIDI/Synthesizer/ToneGenerator.hpp>
#include <LSP/Threading/EventSignal.hpp>

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

private:
	SDL_Window* mWindow = nullptr;

	// 描画スレッド
	std::thread mDrawingThread;
	std::mutex mDrawingMutex;
	std::atomic_bool mDrawingThreadAborted;

	// シーケンサ
	LSP::MIDI::Synthesizer::ToneGenerator mToneGenerator;
	LSP::MIDI::SMF::Sequencer mSequencer;
};

}