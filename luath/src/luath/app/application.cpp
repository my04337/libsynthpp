#include <luath/app/application.hpp>
#include <luath/window/main_window.hpp>

using namespace lsp;
using namespace luath;

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
	// JUCE メッセージマネージャー初期化 ※Win32メッセージループに相当
	auto msgManager = juce::MessageManager::getInstance();

	// メインウィンドウ生成
	{
		luath::window::MainWindow mainWindow;
		check(mainWindow.initialize());

		// メッセージループ開始
		msgManager->runDispatchLoop();
	}

	// メッセージループ停止
	juce::MessageManager::deleteInstance();

	// 削除されていないオブジェクトを全て削除
	// MEMO Win32+WASAPI環境において、デバイスの変更を受け付けるためのスレッドがリークするための措置。
	juce::DeletedAtShutdown::deleteAll();

	return 0;
}
