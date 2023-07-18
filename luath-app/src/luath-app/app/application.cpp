/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/


#include <luath-app/app/application.hpp>
#include <luath-app/app/ume-tgo4.ttf.hpp>
#include <luath-app/window/main_window.hpp>

#include <fstream>

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

	// デフォルトフォントのロード
	mDefaultTypeface = createTypefaceFromCompressedBinary(
		detail::ume_tgo4_ttf_gz_bin.data(),
		detail::ume_tgo4_ttf_gz_bin.size(),
		juce::GZIPDecompressorInputStream::Format::gzipFormat
	);

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
juce::Typeface::Ptr Application::getDefaultTypeface()const noexcept
{
	return mDefaultTypeface;
}
juce::Typeface::Ptr Application::createTypefaceFromCompressedBinary(const void* compressedData, size_t compressedSize, juce::GZIPDecompressorInputStream::Format format)
{
	juce::MemoryInputStream mis(compressedData, compressedSize, /*keepInternalCopyOfData*/false);
	juce::GZIPDecompressorInputStream gzis(&mis, false, format);
	
	juce::MemoryBlock decompressed;
	size_t read = gzis.readIntoMemoryBlock(decompressed);

	return juce::Typeface::createSystemTypefaceFor(decompressed.getData(), decompressed.getSize());
}
