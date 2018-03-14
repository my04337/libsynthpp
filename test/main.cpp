#include <LSP/minimal.hpp>
#include <LSP/Debugging/Logging.hpp>
#include <LSP/Audio/WasapiOutput.hpp>

#ifdef WIN32
#include <objbase.h>
#endif

using namespace LSP;

void func() {
	lsp_debug_log(L"");
}

int main(int argc, char** argv)
{
#ifdef WIN32
	// COM初期化
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	auto fin_act_com = finally([]{CoUninitialize();});
#endif

	// ログ出力機構 セットアップ
	StdOutLogger logger;
	Log::addLogger(&logger);
	auto fin_act_logger = finally([&]{ Log::removeLogger(&logger); });
	Log::setLogLevel(LogLevel::Debug);

	// ---

	Windows::WasapiOutput wo;
	wo.initialize(44100, 16, 2);
	wo.start();
	Sleep(1000);
#if 0
	TaskDispatcher disp(4);

	std::vector<std::unique_ptr<Task>> tasks;
	for (size_t i = 0; i < 4; ++i) {
		tasks.emplace_back(Task::make([i](){lsp_debug_log(string_t(1, 'a'+i)); _sleep(1000);}));
	}
	auto taskId0 = tasks[0]->id();
	disp.enqueue(std::move(tasks[0]));
	disp.enqueue(std::move(tasks[1]));
	disp.enqueue(std::move(tasks[2]));
	disp.enqueue(std::move(tasks[3]), {taskId0});
	
	while(disp.count() > 0) _sleep(100);
#endif

	return 0;
}