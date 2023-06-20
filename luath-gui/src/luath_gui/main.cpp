#include <luath_gui/app/application.hpp>


int main(int argc, char** argv)
{
	// ログ出力機構 セットアップ
#ifdef WIN32
	lsp::OutputDebugStringLogger logger;
#else
	StdOutLogger logger;
#endif
	lsp::Log::addLogger(&logger);
	auto fin_act_logger = lsp::finally([&]{ lsp::Log::removeLogger(&logger); });
	lsp::Log::setLogLevel(lsp::LogLevel::Debug);
	
	// スタート
	return luath_gui::Application::instance().exec(argc, argv);
}