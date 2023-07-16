/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/oscilloscope.hpp>
#include <luath-app/widget/spectrum_analyzer.hpp>
#include <luath-app/widget/lissajous.hpp>

namespace lsp::synth { class LuathSynth; }
namespace luath::app
{

class MainContent
	: public juce::OpenGLAppComponent
{
public:
	MainContent(const lsp::synth::LuathSynth& synth, const juce::AudioDeviceManager& audioDeviceManager);
	~MainContent()override;

	void audioDeviceAboutToStart(juce::AudioIODevice* device);
	void writeAudio(const lsp::Signal<float>& sig);

	void initialise()override;
	void shutdown()override;
	void render()override;

	void paint(juce::Graphics& g)override;

private:
	const lsp::synth::LuathSynth& mSynth;
	const juce::AudioDeviceManager& mAudioDeviceManager;

	// 描画関連
	juce::Typeface::Ptr mDefaultTypeface;
	juce::Font mDefaultFont;
	juce::Font mSmallFont;

	// ウィジット類
	widget::OscilloScope mOscilloScopeWidget;
	widget::SpectrumAnalyzer mSpectrumAnalyzerWidget;
	widget::Lissajous mLissajousWidget;

	// FPS計算,表示用
	size_t mFrames = 0;
	std::array<std::chrono::microseconds, 15> mDrawingTimeHistory = {};
	std::array<std::chrono::microseconds, 15> mFrameIntervalHistory = {};
	clock::time_point mPrevDrawingStartTime = clock::now();
	size_t mDrawingTimeIndex = 0;
};

//
}