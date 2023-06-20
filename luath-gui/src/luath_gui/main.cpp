#include <luath_gui/app/application.hpp>


using namespace LSP;

int main(int argc, char** argv)
{
	// ログ出力機構 セットアップ
	Win32::OutputDebugStringLogger logger;
	Log::addLogger(&logger);
	auto fin_act_logger = finally([&]{ Log::removeLogger(&logger); });
	Log::setLogLevel(LogLevel::Debug);
	
	// スタート
	return Luath::Application::instance().exec(argc, argv);
}