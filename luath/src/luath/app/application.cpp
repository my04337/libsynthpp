#include <luath/app/application.hpp>
#include <luath/window/main_window.hpp>

using namespace lsp;
using namespace luath;

Application::Application(int argc, char** argv)
{
	check(SUCCEEDED(CoInitialize(NULL)));

	// 実行ファイルのパスを取得
	{
		std::vector<TCHAR> buff;
		buff.resize(MAX_PATH + 1, _TEXT('\0'));
		GetModuleFileName(nullptr, buff.data(), MAX_PATH);
		mProgramPath = std::filesystem::path(buff.data()).parent_path();
	}	

}
Application::~Application()
{
	CoUninitialize();
}

int Application::exec()
{
	// ウィンドウ生成
	luath::window::MainWindow mainWindow(*this);
    check(mainWindow.initialize());
    
	// メッセージループ 開始
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

const std::filesystem::path& Application::programPath()const noexcept
{
	return mProgramPath;
}