﻿/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/window/main_content.hpp>
#include <luath-app/app/application.hpp>
#include <lsp/synth/synth.hpp>

using namespace std::string_literals;
using namespace luath::app;
using namespace luath::app::widget;

using lsp::synth::LuathVoice;
using lsp::synth::VoiceId;

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 680;

static constexpr juce::Colour getMidiChannelColor(int ch)
{
	switch(ch) {
	case  1: return juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 1.f); // 赤
	case  2: return juce::Colour::fromFloatRGBA(1.f, 0.5f, 0.f, 1.f); // 朱色
	case  3: return juce::Colour::fromFloatRGBA(1.f, 0.75f, 0.f, 1.f); // ゴールデンイエロー
	case  4: return juce::Colour::fromFloatRGBA(1.f, 1.f, 0.f, 1.f); // 黄色
	case  5: return juce::Colour::fromFloatRGBA(0.75f, 1.f, 0.f, 1.f); // 明るい黄緑色 
	case  6: return juce::Colour::fromFloatRGBA(0.f, 1.f, 0.f, 1.f); // 緑
	case  7: return juce::Colour::fromFloatRGBA(0.f, 1.f, 0.75f, 1.f); // 黄緑色
	case  8: return juce::Colour::fromFloatRGBA(0.f, 0.75f, 1.f, 1.f); // セルリアンブルー
	case  9: return juce::Colour::fromFloatRGBA(0.f, 0.3f, 1.f, 1.f); // コバルトブルー
	case 10: return juce::Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 1.f); // グレー ※通常ドラム
	case 11: return juce::Colour::fromFloatRGBA(0.3f, 0.f, 1.f, 1.f); // ヒヤシンス
	case 12: return juce::Colour::fromFloatRGBA(0.5f, 0.f, 1.f, 1.f); // バイオレット
	case 13: return juce::Colour::fromFloatRGBA(0.75f, 0.f, 1.f, 1.f); // ムラサキ
	case 14: return juce::Colour::fromFloatRGBA(1.f, 0.f, 1.f, 1.f); // マゼンタ
	case 15: return juce::Colour::fromFloatRGBA(1.f, 0.f, 0.5f, 1.f); // ルビーレッド
	case 16: return juce::Colour::fromFloatRGBA(0.75f, 0.f, 0.3f, 1.f); // カーマイン
	}
	std::unreachable();
};


static constexpr const wchar_t* state2text(LuathVoice::EnvelopeState state)
{
	switch(state) {
	case LuathVoice::EnvelopeState::Attack: return L"Attack";
	case LuathVoice::EnvelopeState::Hold:   return L"Hold";
	case LuathVoice::EnvelopeState::Decay:  return L"Decay";
	case LuathVoice::EnvelopeState::Fade:   return L"Fade";
	case LuathVoice::EnvelopeState::Release:return L"Release";
	case LuathVoice::EnvelopeState::Free:   return L"Free";
	default: return L"Unknown";
	}
};
static std::wstring freq2scale(float freq) {
	// MEMO 表示用。 ひとまず平均律で算出している
	//mCalculatedFreq = 440 * exp2((static_cast<float>(mNoteNo) + mPitchBend - 69.0f) / 12.0f);

	static constexpr std::array<const wchar_t*, 128> scales {
		L"C-1", L"C#-1", L"D-1", L"D#-1", L"E-1", L"F-1", L"F#-1", L"G-1", L"G#-1", L"A-1", L"A#-1", L"B-1",
			L"C0", L"C#0", L"D0", L"D#0", L"E0", L"F0", L"F#0", L"G0", L"G#0", L"A0", L"A#0", L"B0",
			L"C1", L"C#1", L"D1", L"D#1", L"E1", L"F1", L"F#1", L"G1", L"G#1", L"A1", L"A#1", L"B1",
			L"C2", L"C#2", L"D2", L"D#2", L"E2", L"F2", L"F#2", L"G2", L"G#2", L"A2", L"A#2", L"B2",
			L"C3", L"C#3", L"D3", L"D#3", L"E3", L"F3", L"F#3", L"G3", L"G#3", L"A3", L"A#3", L"B3",
			L"C4", L"C#4", L"D4", L"D#4", L"E4", L"F4", L"F#4", L"G4", L"G#4", L"A4", L"A#4", L"B4",
			L"C5", L"C#5", L"D5", L"D#5", L"E5", L"F5", L"F#5", L"G5", L"G#5", L"A5", L"A#5", L"B5",
			L"C6", L"C#6", L"D6", L"D#6", L"E6", L"F6", L"F#6", L"G6", L"G#6", L"A6", L"A#6", L"B6",
			L"C7", L"C#7", L"D7", L"D#7", L"E7", L"F7", L"F#7", L"G7", L"G#7", L"A7", L"A#7", L"B7",
			L"C8", L"C#8", L"D8", L"D#8", L"E8", L"F8", L"F#8", L"G8", L"G#8", L"A8", L"A#8", L"B8",
			L"C9", L"C#9", L"D9", L"D#9", L"E9", L"F9", L"F#9", L"G9",
	};

	auto rawNoteNo = log2(freq / 440.f) * 12.f + 69.f;
	auto roundedNoteNo = static_cast<int32_t>(rawNoteNo);

	if(roundedNoteNo < 0) {
		return L"<LOW>";
	}
	else if(roundedNoteNo > 128) {
		return L"<HIGH>";
	}
	else [[likely]] {
		return scales[roundedNoteNo];
	}
}

MainContent::MainContent(const lsp::synth::LuathSynth& synth, const juce::AudioDeviceManager& audioDeviceManager)
	: mSynth(synth)
	, mAudioDeviceManager(audioDeviceManager)
{
	auto app = dynamic_cast<luath::app::Application*>(juce::JUCEApplication::getInstance());
	check(app != nullptr);

	mDefaultFont = juce::Font(app->getDefaultTypeface());
	mDefaultFont.setHeight(12);
	mSmallFont = juce::Font(app->getDefaultTypeface());
	mSmallFont.setHeight(10);

	// OpenGL関連初期化
	// MEMO デフォルトではソフトウェアレンダリングとなるが、これがとても遅く実用に耐えないためOpenGLを利用している
	openGLContext.setComponentPaintingEnabled(true);

	// 子コンポーネントのセットアップ
	const int margin = 5; // unsecaledx
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


	// チャネル毎の合計発音数を取得
	std::array<int, 16> poly;
	std::fill(poly.begin(), poly.end(), 0);
	for(auto& vd : voiceDigests) {
		if(vd.ch < 1 || vd.ch > 16) continue;
		if(vd.state == LuathVoice::EnvelopeState::Free) continue;
		++poly[vd.ch - 1];
	}
	int polyCount = std::accumulate(poly.begin(), poly.end(), 0);

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

	// チャネル情報
	{
		const float ofsX = 10;
		const float ofsY = 60;
		const float height = 15;

		constexpr std::array<float, 10> columnWidth{ 25, 75, 40, 40, 60, 35, 75, 25, 25, 25 };
		float x = ofsX;
		size_t ci = 0;
		auto col = [&] {
			float ret = x;
			x += columnWidth[ci++];
			return ret;
		};
		float y = ofsY;
		constexpr float unscaled_width = std::accumulate(columnWidth.begin(), columnWidth.end(), 0.f);
		const float width = unscaled_width;

		{
			x = ofsX;
			ci = 0;
			drawText(col(), y, L"Ch");
			drawText(col(), y, L"Ins");
			drawText(col(), y, L"Vol");
			drawText(col(), y, L"Exp");
			drawText(col(), y, L"Pitch");
			drawText(col(), y, L"Pan");
			drawText(col(), y, L"Envelope");
			drawText(col(), y, L"Ped");
			drawText(col(), y, L"Drm");
			drawText(col(), y, L"Poly");
		}
		for(const auto& cd : channelDigests) {
			y += height;
			x = ofsX;
			ci = 0;

			g.setColour(getMidiChannelColor(cd.ch).withMultipliedAlpha(0.5f));
			g.fillRect(x, y, width , height);

			drawText(col(), y, std::format(L"{:02}", cd.ch));
			drawText(col(), y, std::format(L"{:03}:{:03}.{:03}", cd.progId, cd.bankSelectMSB, cd.bankSelectLSB));
			drawText(col(), y, std::format(L"{:0.3f}", cd.volume));
			drawText(col(), y, std::format(L"{:0.3f}", cd.expression));
			drawText(col(), y, std::format(L"{:+0.4f}", cd.pitchBend));
			drawText(col(), y, std::format(L"{:0.2f}", cd.pan));
			drawText(col(), y, std::format(L"{:03}.{:03}.{:03}",
				static_cast<int>(std::round(cd.attackTime * 127)),
				static_cast<int>(std::round(cd.decayTime * 127)),
				static_cast<int>(std::round(cd.releaseTime * 127))));
			drawText(col(), y, cd.pedal ? L"on" : L"off");
			drawText(col(), y, cd.drum ? L"on" : L"off");
			drawText(col(), y, std::format(L"{:02}", poly[cd.ch - 1]));
		}
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