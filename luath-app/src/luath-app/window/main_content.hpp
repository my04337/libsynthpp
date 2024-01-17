/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/widget/general_info.hpp>
#include <luath-app/widget/channel_info.hpp>
#include <luath-app/widget/voice_info.hpp>
#include <luath-app/widget/oscilloscope.hpp>
#include <luath-app/widget/spectrum_analyzer.hpp>
#include <luath-app/widget/lissajous.hpp>

namespace lsp::synth { class LuathSynth; }
namespace luath::app
{

class MainContent
	: public juce::Component
{
public:
	MainContent(const lsp::synth::LuathSynth& synth, const juce::AudioDeviceManager& audioDeviceManager);
	~MainContent()override;

	void audioDeviceAboutToStart(juce::AudioIODevice* device);
	void writeAudio(const lsp::Signal<float>& sig);

private:
	const lsp::synth::LuathSynth& mSynth;
	const juce::AudioDeviceManager& mAudioDeviceManager;

	// 描画関連
	juce::Font mDefaultFont;
	juce::Font mSmallFont;

	// ウィジット類
	widget::GeneralInfo mGeneralInfoWidget;
	widget::ChannelInfo mChannelInfoWidget;
	widget::VoiceInfo mVoiceInfoWidget;
	widget::OscilloScope mOscilloScopeWidget;
	widget::SpectrumAnalyzer mSpectrumAnalyzerWidget;
	widget::Lissajous mLissajousWidget;
};

//
}