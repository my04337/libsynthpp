// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#include <luath-app/app/application.hpp>
#include <luath-app/app/ume-tgo4.ttf.hpp>
#include <luath-app/window/main_window.hpp>

#include <fstream>

using namespace luath::app;

Application::~Application() = default;

void Application::initialise(const juce::String& commandLineParameters)
{
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
}

const juce::String Application::getApplicationName()
{
	return L"luath - LibSynth++ Sample MIDI Synthesizer";
}
const juce::String Application::getApplicationVersion()
{
	return L"";
}
juce::Typeface::Ptr Application::createDefaultTypeface()const noexcept
{
	// デフォルトフォントのロード
	return createTypefaceFromCompressedBinary(
		detail::ume_tgo4_ttf_gz_bin.data(),
		detail::ume_tgo4_ttf_gz_bin.size(),
		juce::GZIPDecompressorInputStream::Format::gzipFormat
	);
}
juce::Typeface::Ptr Application::createTypefaceFromCompressedBinary(const void* compressedData, size_t compressedSize, juce::GZIPDecompressorInputStream::Format format)
{
	juce::MemoryInputStream mis(compressedData, compressedSize, /*keepInternalCopyOfData*/false);
	juce::GZIPDecompressorInputStream gzis(&mis, false, format);
	
	juce::MemoryBlock decompressed;
	size_t read = gzis.readIntoMemoryBlock(decompressed);

	return juce::Typeface::createSystemTypefaceFor(decompressed.getData(), decompressed.getSize());
}
