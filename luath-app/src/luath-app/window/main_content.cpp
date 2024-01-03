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

	// OpenGL関連初期化
	// MEMO デフォルトではソフトウェアレンダリングとなるが、これがとても遅く実用に耐えないためOpenGLを利用している
	openGLContext.setComponentPaintingEnabled(true);

	// 子コンポーネントのセットアップ
	const int margin = 5; // unsecaled

	mChannelInfoWidget.setBounds(
		10,
		30,
		435,
		255
	);
	addAndMakeVisible(mChannelInfoWidget);

	mLissajousWidget.setBounds(
		10 + margin,
		340 + margin,
		150 - margin * 2, 
		150 - margin * 2
		);
	addAndMakeVisible(mLissajousWidget);

	mOscilloScopeWidget.setBounds(
		160 + margin,
		340 + margin,
		300 - margin * 2,
		150 - margin * 2
	);
	addAndMakeVisible(mOscilloScopeWidget);

	mSpectrumAnalyzerWidget.setBounds(
		10 + margin,
		490 + margin,
		450 - margin * 2,
		150 - margin * 2
	);
	addAndMakeVisible(mSpectrumAnalyzerWidget);


	// その他ウィンドウ設定
	setSize(SCREEN_WIDTH, SCREEN_HEIGHT);
}

MainContent::~MainContent()
{
	shutdownOpenGL();
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
	mChannelInfoWidget.update(digest);
}

void MainContent::initialise()
{

}
void MainContent::shutdown()
{

}
void MainContent::render()
{
	repaint();
}

void MainContent::paint(juce::Graphics& g)
{

	// 描画時間記録開始
	auto drawingStartTime = clock::now();
	mFrameIntervalHistory[mDrawingTimeIndex] = std::chrono::duration_cast<std::chrono::microseconds>(drawingStartTime - mPrevDrawingStartTime);
	mPrevDrawingStartTime = drawingStartTime;

	// 描画対象のサイズ取得
	auto windowHandle = reinterpret_cast<HWND>(getWindowHandle());
	SIZE windowSize{ getWidth(), getHeight() };
	auto drawingScale = static_cast<float>(GetDpiForWindow(windowHandle)) / 96.f;

	// 描画開始
	g.saveState();
	auto fin_act_restore_state = finally([&] {g.restoreState(); });

	auto drawText = [&](float x, float y, std::wstring_view str) {
		g.setColour(juce::Colours::black);
		g.setFont(mDefaultFont);
		g.drawSingleLineText(
			juce::String(str.data(), str.size()),
			static_cast<int>(x),
			static_cast<int>(y + mDefaultFont.getAscent())
		);
	};
	auto drawSmallText = [&](float x, float y, std::wstring_view str) {
		g.setColour(juce::Colours::black);
		g.setFont(mSmallFont);
		g.drawSingleLineText(
			juce::String(str.data(), str.size()),
			static_cast<int>(x),
			static_cast<int>(y + mSmallFont.getAscent())
		);
	};

	// 各種情報取得
	const auto tgStatistics = mSynth.statistics();
	const auto synthDigest = mSynth.digest();
	const auto& channelDigests = synthDigest.channels;
	const auto& voiceDigests = synthDigest.voices;
	int polyCount = 0;
	for(auto& vd : voiceDigests) {
		if(vd.ch < 1 || vd.ch > 16) continue;
		if(vd.state == LuathVoice::EnvelopeState::Free) continue;
		++polyCount;
	}

	// 背景塗りつぶし
	g.fillAll(juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 1.f));

	// 基本情報
	{
		auto average_interval_time = std::accumulate(mFrameIntervalHistory.cbegin(), mFrameIntervalHistory.cend(), std::chrono::microseconds(0)) / mFrameIntervalHistory.size();
		auto average_drawing_time = std::accumulate(mDrawingTimeHistory.cbegin(), mDrawingTimeHistory.cend(), std::chrono::microseconds(0)) / mDrawingTimeHistory.size();
		if(average_interval_time.count() > 0) {
			auto drawing_load = static_cast<float>(average_drawing_time.count()) / static_cast<float>(average_interval_time.count());
			drawText(0, 0, std::format(L"描画時間 : {:05.2f}[msec]", average_drawing_time.count() / 1000.f));
			drawText(0, 15, std::format(L"描画負荷 : {:06.2f}[%]", 100 * drawing_load));
		}

	}
	// 演奏情報
	{
		auto systemType = synthDigest.systemType.toPrintableWString();
		if(auto audioDevice = mAudioDeviceManager.getCurrentAudioDevice()) {
			auto sampleFreq = audioDevice->getCurrentSampleRate();
			auto buffered = audioDevice->getCurrentBufferSizeSamples() / sampleFreq;
			auto latency = audioDevice->getOutputLatencyInSamples() / sampleFreq;

			drawText(150, 0, std::format(L"バッファ : {:04}[msec]  レイテンシ : {:04}[msec]  デバイス名 : {}  ",
				buffered * 1000,
				latency * 1000,
				audioDevice->getName().toWideCharPointer()
			));
		}

		drawText(150, 15, std::format(L"演奏負荷 : {:06.2f}[%]", 100 * mAudioDeviceManager.getCpuUsage()));
		drawText(0, 30, std::format(L"マスタ音量 : {:.3f}", synthDigest.masterVolume));
		drawText(280, 15, std::format(L"同時発音数 : {:03}", polyCount));
		drawText(420, 15, std::format(L"MIDIリセット : {}", systemType));
	}

	// ボイス情報 (折り返し表示)
	{
		float ofsX = 460;
		const float ofsY = 60;
		const size_t voicePerRow = 45;
		const float height = 12;

		constexpr std::array<float, 4> columnWidth{ 15, 22, 30, 40};
		float x = ofsX;
		size_t ci = 0;
		auto col = [&] {
			float ret = x;
			x += columnWidth[ci++];
			return ret;
		};
		constexpr float unscaled_width = std::accumulate(columnWidth.begin(), columnWidth.end(), 0.f);
		const float width = unscaled_width;
		{
			drawText(col(), ofsY, L"Ch");
			drawText(col(), ofsY, L"Sc.");
			drawText(col(), ofsY, L"Env.");
			drawText(col(), ofsY, L"Status");
		}
		for(size_t i = 0; i < voiceDigests.size(); ++i) {
			const auto& vd = voiceDigests[i];
			if(vd.state == LuathVoice::EnvelopeState::Free) continue;

			x = ofsX + (static_cast<float>(i / voicePerRow)) * (width + 10);
			float y = ofsY + height * ((i % voicePerRow) + 1);
			ci = 0;

			g.setColour(getMidiChannelColor(vd.ch).withMultipliedAlpha(std::clamp(vd.envelope * 0.4f + 0.1f, 0.f, 1.f)));
			g.fillRect(x, y, width, height);

			drawSmallText(col(), y, std::format(L"{:02}", vd.ch));
			drawSmallText(col(), y, freq2scale(vd.freq));
			drawSmallText(col(), y, std::format(L"{:.3f}", vd.envelope));
			drawSmallText(col(), y, state2text(vd.state));
		}
	}
	// 子ウィジット描画
	mLissajousWidget.paint(g);
	mOscilloScopeWidget.paint(g);
	mSpectrumAnalyzerWidget.paint(g);


	// 描画終了
	auto drawingEndTime = clock::now();
	mDrawingTimeHistory[mDrawingTimeIndex] = std::chrono::duration_cast<std::chrono::microseconds>(drawingEndTime - drawingStartTime);
	++mDrawingTimeIndex;
	if(mDrawingTimeIndex == mDrawingTimeHistory.size()) {
		mDrawingTimeIndex = 0;
	}
	++mFrames;
}