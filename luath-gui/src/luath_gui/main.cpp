#include <luath_gui/app/application.hpp>

using namespace lsp;
using namespace luath_gui;

int WINAPI WinMain(
	HINSTANCE /* hInstance */,
	HINSTANCE /* hPrevInstance */,
	LPSTR /* lpCmdLine */,
	int /* nCmdShow */
)
{
	// ログ出力機構 セットアップ
	OutputDebugStringLogger logger;
	Log::addLogger(&logger);
	auto fin_act_logger = finally([&] { lsp::Log::removeLogger(&logger); });
	Log::setLogLevel(lsp::LogLevel::Debug);

	// COM初期化
	check(SUCCEEDED(CoInitialize(NULL)));
	auto fin_act_com = finally([] {CoUninitialize(); });

	// アプリケーション起動
	return Application::instance().exec(__argc, __argv);
}