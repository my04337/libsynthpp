#include <LSP/minimal.hpp>
#include "case/test_base.hpp"
#include "case/test_wasapi.hpp"
#include "case/test_smf.hpp"

#include <iostream>

#ifdef WIN32
#include <objbase.h>
#endif

using namespace LSP;

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

	// テスト登録
	std::vector<std::unique_ptr<Test::ITest>> tests;
	
	tests.emplace_back(std::make_unique<Test::BaseTest>());
	tests.emplace_back(std::make_unique<Test::WasapiTest>());
	tests.emplace_back(std::make_unique<Test::MidiSmfTest>());

	// テスト実行

	//exec
	while(true){
		std::cout << "---Select Test---" << std::endl;
		std::cout << "* all : exec all tests." << std::endl;
		for(auto& test : tests){
			std::cout << "* " << test->command() << " : " << test->description() << std::endl;
		} 
		std::cout << "* q : quit test" << std::endl << std::endl;

		char buf[256] = "\0";
		bool executed = false;
		while(true){
			std::cout << ":";
			if(!gets_s(buf, sizeof(buf)-1) || strlen(buf))
				break;
		}
		std::string_view cmd(buf);

		if(cmd == "q") {
			break;
		}

		for(auto& test : tests) {
			if(cmd == "all" || cmd == test->command()) {
				test->exec();
				executed = true;
			}
		}
		if(!executed) {
			std::cout << std::endl << "Error : command \"" << cmd << "\" is not found." << std::endl;
		} else {
			std::cout << std::endl << "finished." << std::endl << std::endl;
		}
	}

	return 0;
}