#include <Luath/App/Application.hpp>
#include <Luath/App/FontCache.hpp>
#include <Luath/Window/MainWindow.hpp>

using namespace LSP;
using namespace Luath;

Application::Application()
{
	// SDL 初期化
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		Log::f(LOGF("Main : could not initialize SDL :" << SDL_GetError()));
	}
	if (TTF_Init() < 0) {
		Log::f(LOGF("Main : could not initialize SDL_ttf :" << TTF_GetError()));
	}

	// 各種サービス初期化
	mFontCache = std::make_unique<FontCache>();
}
Application::~Application()
{
	mFontCache.reset();
	TTF_Quit();
	SDL_Quit();
}

Application& Application::instance()
{
	static Application app;
	return app;
}

int Application::exec(int argc, char** argv)
{

	// ウィンドウ生成
	Luath::Window::MainWindow main_window;
	if(!main_window.initialize()) {
		return 1;
	}

	// メッセージループ 開始
	bool done = false;
	SDL_Event ev;
	while (!done && SDL_WaitEvent(&ev))
	{
		switch (ev.type) {
		case SDL_KEYDOWN:
			main_window.onKeyDown(ev.key);
			break;
		case SDL_DROPFILE:
			main_window.onDropFile(ev.drop);
			break;
		case SDL_QUIT:
			main_window.dispose();
			done = true;
			break;
		}
	}
	return 0;
}

FontCache& Application::fontCache()noexcept
{
	lsp_assert(mFontCache != nullptr);
	return *mFontCache;
}