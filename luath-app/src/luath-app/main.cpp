#include <luath-app/app/application.hpp>

using namespace lsp;
using namespace luath;

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

	// アプリケーション初期化
	Application app(__argc, __argv);

	// アプリケーション起動
	return app.exec();
}