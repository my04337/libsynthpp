#include <Luath/Window/MainWindow.hpp>

using namespace LSP;
using namespace Luath;
using namespace Luath::Window;

MainWindow::MainWindow()
{
}

MainWindow::~MainWindow()
{
	if (mWindow) {
		SDL_DestroyWindow(mWindow);
	}
}
bool MainWindow::initialize()
{
	constexpr int SCREEN_WIDTH = 600;
	constexpr int SCREEN_HEIGHT = 400;

	auto window = SDL_CreateWindow(
		"luath - LibSynth++ Sample MIDI Synthesizer",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_OPENGL
	);
	if(!window) {
		Log::e(LOGF("Main : could not create window :" << SDL_GetError()));
		return false;
	}
	auto fail_act_destroy = finally([&]{SDL_DestroyWindow(window);});


	// OK
	mWindow = window;
	fail_act_destroy.reset();
	return true;
}