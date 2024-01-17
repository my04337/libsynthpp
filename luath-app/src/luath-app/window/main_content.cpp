/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/window/main_content.hpp>
#include <luath-app/app/application.hpp>
#include <luath-app/app/util.hpp>
#include <lsp/synth/synth.hpp>

using namespace std::string_literals;
using namespace luath::app;
using namespace luath::app::widget;

using lsp::synth::LuathVoice;
using lsp::synth::VoiceId;

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 680;


MainContent::MainContent(const lsp::synth::LuathSynth& synth, const juce::AudioDeviceManager& audioDeviceManager)
	: mSynth(synth)
	, mAudioDeviceManager(audioDeviceManager)
{
	auto app = dynamic_cast<luath::app::Application*>(juce::JUCEApplication::getInstance());
	check(app != nullptr);

	auto typeface = app->createDefaultTypeface();
	mDefaultFont = juce::Font(typeface);
	mDefaultFont.setHeight(12);
	mSmallFont = juce::Font(typeface);
	mSmallFont.setHeight(10);

	// 親ウィンドウに関係するセットアップ
	setSize(SCREEN_WIDTH, SCREEN_HEIGHT);

	// 子コンポーネントのセットアップ
	const int margin = 5; // unsecaled

	mGeneralInfoWidget.setBounds(0 + margin, +margin, SCREEN_WIDTH * margin * 2, 60 - margin * 2);
	addAndMakeVisible(mGeneralInfoWidget);

	mChannelInfoWidget.setBounds(10, 60, 435, 255);
	addAndMakeVisible(mChannelInfoWidget);

	mVoiceInfoWidget.setBounds(460, 60, 214, 540);
	addAndMakeVisible(mVoiceInfoWidget);

	mLissajousWidget.setBounds(10 + margin, 340 + margin, 150 - margin * 2,  150 - margin * 2);
	addAndMakeVisible(mLissajousWidget);

	mOscilloScopeWidget.setBounds(160 + margin, 340 + margin, 300 - margin * 2, 150 - margin * 2);
	addAndMakeVisible(mOscilloScopeWidget);

	mSpectrumAnalyzerWidget.setBounds(10 + margin, 490 + margin, 450 - margin * 2, 150 - margin * 2);
	addAndMakeVisible(mSpectrumAnalyzerWidget);
}

MainContent::~MainContent()
{
}


void MainContent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
	if(device) {
		auto sampleFreq = static_cast<float>(device->getCurrentSampleRate());
		mOscilloScopeWidget.setParams(sampleFreq, 25e-3f);
		mSpectrumAnalyzerWidget.setParams(sampleFreq, 4096, 2);
		mLissajousWidget.setParams(sampleFreq, 25e-3f);
	}	
}

void MainContent::writeAudio(const lsp::Signal<float>& sig)
{
	mOscilloScopeWidget.write(sig);
	mSpectrumAnalyzerWidget.write(sig);
	mLissajousWidget.write(sig);


	auto digest = std::make_shared<lsp::synth::LuathSynth::Digest>(mSynth.digest());
	mGeneralInfoWidget.update(mAudioDeviceManager, digest);
	mChannelInfoWidget.update(digest);
	mVoiceInfoWidget.update(digest);
}
