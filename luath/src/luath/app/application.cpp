#include <luath/app/application.hpp>
#include <luath/window/main_window.hpp>

using namespace lsp;
using namespace luath;

Application::Application(int argc, char** argv)
{
	lsp_check(SUCCEEDED(CoInitialize(NULL)));
}
Application::~Application()
{
	CoUninitialize();
}

int Application::exec()
{
	// ウィンドウ生成
	luath::window::MainWindow main_window;
	lsp_check(main_window.initialize());
    
	// メッセージループ 開始

	MSG msg;

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
