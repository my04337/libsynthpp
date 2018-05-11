﻿#include <Luath/App/Application.hpp>
#include <Luath/Window/MainWindow.hpp>

#include <SDL.h>
#include <SDL_ttf.h>

using namespace LSP;
using namespace Luath;

Application::Application()
{
}

Application& Application::instance()
{
	static Application app;
	return app;
}

int Application::exec()
{
	// SDL 初期化
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		Log::e(LOGF("Main : could not initialize SDL :" << SDL_GetError()));
		return 1;
	}
	auto fin_act_sdl = finally([]{SDL_Quit();});
	if (TTF_Init() < 0) {
		Log::e(LOGF("Main : could not initialize SDL_ttf :" << TTF_GetError()));
		return 1;
	}
	auto fin_act_sdl_ttf = finally([]{TTF_Quit();});


	// ウィンドウ生成
	Luath::Window::MainWindow main_window;
	if (!main_window.initialize()) {
		return 1;
	}

	// メッセージループ 開始
	bool done = false;
	SDL_Event ev;
	while (!done && SDL_WaitEvent(&ev))
	{
		switch (ev.type) {
		case SDL_QUIT:
			done = true;
			break;
		}
	}
	return 0;
}