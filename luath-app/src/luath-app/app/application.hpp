/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

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

	juce::Typeface::Ptr getDefaultTypeface()const noexcept;

private:
	std::list<std::unique_ptr<ILogger>> mLoggers;
	std::unique_ptr<juce::DocumentWindow> mMainWindow;
	juce::Typeface::Ptr mDefaultTypeface;
};

}