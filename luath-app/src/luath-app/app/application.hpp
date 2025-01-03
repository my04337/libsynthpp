// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <luath-app/core/core.hpp>

namespace luath::app
{
class Application final
	: public juce::JUCEApplication
{
public:
	~Application()override;

	void initialise(const juce::String& commandLineParameters)override;
	void shutdown()override;

	const juce::String getApplicationName()override;
	const juce::String getApplicationVersion()override;

	juce::Typeface::Ptr createDefaultTypeface()const noexcept;

private:
	static juce::Typeface::Ptr createTypefaceFromCompressedBinary(const void* compressedData, size_t compressedSize, juce::GZIPDecompressorInputStream::Format format);

private:
	std::list<std::unique_ptr<ILogger>> mLoggers;
	std::unique_ptr<juce::DocumentWindow> mMainWindow;
};

}