#include <luath_gui/app/application.hpp>
#include <luath_gui/window/main_window.hpp>

using namespace lsp;
using namespace luath_gui;

Application::Application(int argc, char** argv)
{
	check(SUCCEEDED(CoInitialize(NULL)));
}
Application::~Application()
{
	CoUninitialize();
}

int Application::exec()
{
	// ウィンドウ生成
	luath_gui::window::MainWindow main_window;
    check(main_window.initialize());
    
	// メッセージループ 開始

	MSG msg;

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
