#include <LSP/minimal.hpp>
#include <LSP/Debugging/Logging.hpp>

using namespace LSP;

void func() {
	lsp_debug_log(L"");
}

int main(int argc, char** argv)
{
	// ログ出力機構 セットアップ
	StdOutLogger logger;
	Log::addLogger(&logger);
	auto fin_act_logger = finally([&]{ Log::removeLogger(&logger); });
	Log::setLogLevel(LogLevel::Debug);

	// ---

	return 0;
}