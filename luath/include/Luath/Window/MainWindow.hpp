#pragma once

#include <Luath/Base/Base.hpp>
#include <Luath/Syntesizer/ToneGenerator.hpp>
#include <LSP/MIDI/SMF/Sequencer.hpp>
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
	void playingThreadMain();

private:
	SDL_Window* mWindow = nullptr;

	// 描画スレッド
	std::thread mDrawingThread;
	std::mutex mDrawingMutex;
	std::atomic_bool mDrawingThreadAborted;

	// 演奏スレッド
	std::thread mPlayingThread;
	std::mutex mPlayingMutex;
	std::atomic_bool mPlayingThreadAborted;

	// シーケンサ
	Luath::Synthesizer::LuathToneGenerator mToneGenerator;
	LSP::MIDI::SMF::Sequencer mSequencer;
};

}