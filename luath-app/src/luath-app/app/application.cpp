/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/


#include <luath-app/app/application.hpp>
#include <luath-app/window/main_window.hpp>

using namespace luath::app;

Application::~Application() = default;

void Application::initialise(const juce::String& commandLineParameters)
{
	// Win32 : COM セットアップ
#ifdef WIN32
	check(SUCCEEDED(CoInitialize(NULL)));
#endif

	// ログ出力機構 セットアップ
	auto logger = std::make_unique<OutputDebugStringLogger>();
	Log::addLogger(logger.get());
	mLoggers.emplace_back(std::move(logger));

	Log::setLogLevel(lsp::LogLevel::Debug);

	// メインウィンドウ表示
	mMainWindow = std::make_unique<MainWindow>();
	mMainWindow->setVisible(true);

}
void Application::shutdown()
{
	// メインウィンドウclose
	mMainWindow.reset();

	// ログ出力機構 シャットダウン
	for(auto& logger : mLoggers) {
		Log::removeLogger(logger.get());
	}
	mLoggers.clear();

	// Win32 : COM シャットダウン
#ifdef WIN32
	CoUninitialize();
#endif
}

const juce::String Application::getApplicationName()
{
	return L"luath - LibSynth++ Sample MIDI Synthesizer";
}
const juce::String Application::getApplicationVersion()
{
	return L"";
}
