#include <luath_gui/app/application.hpp>
#include <luath_gui/app/font_cache.hpp>
#include <luath_gui/window/main_window.hpp>

using namespace lsp;
using namespace luath_gui;

Application::Application()
{
	// SDL 初期化
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		Log::f([](auto& _) {_ << "Main : could not initialize SDL :" << SDL_GetError(); });
	}
	if (TTF_Init() < 0) {
		Log::f([](auto& _) {_ << "Main : could not initialize SDL_ttf :" << TTF_GetError(); });
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
	luath_gui::window::MainWindow main_window;
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
	Assertion::check(mFontCache != nullptr);
	return *mFontCache;
}