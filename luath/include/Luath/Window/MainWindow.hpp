#pragma once

#include <Luath/Base/Base.hpp>
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

	// •`‰æƒXƒŒƒbƒh
	std::thread mDrawingThread;
	std::mutex mDrawingMutex;
	std::atomic_bool mDrawingThreadAborted;
};

}