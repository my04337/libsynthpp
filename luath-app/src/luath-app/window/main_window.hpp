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
#include <luath-app/drawing/font_loader.hpp>
#include <lsp/synth/synth.hpp>
#include <lsp/midi/sequencer.hpp>

namespace luath::app::window
{

class MainWindow final
	: non_copy_move
	, juce::AudioIODeviceCallback
	, juce::MidiInputCallback
{
public:
	MainWindow();
	~MainWindow();

	bool initialize();
	void dispose();
	bool onKeyDown(uint16_t key, bool shift, bool ctrl, bool alt);
	void onDropFile(const std::vector<std::filesystem::path>& paths);
	void onDpiChanged(float scale);

private:
	void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)override;

	void audioDeviceIOCallbackWithContext(
		const float* const* inputChannelData,
		int numInputChannels,
		float* const* outputChannelData,
		int numOutputChannels,
		int numSamples,
		const juce::AudioIODeviceCallbackContext& context
	)override;
	void audioDeviceAboutToStart(juce::AudioIODevice* device)override;
	void audioDeviceStopped()override;
	void audioDeviceError(const juce::String& errorMessage)override;

protected:
	void loadMidi(const std::filesystem::path& path);
	void onDraw();
	void onDraw(ID2D1RenderTarget& renderer);


private:
	static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	juce::CriticalSection mMidiBufferLock;
	juce::MidiBuffer mMidiBuffer;

	HWND mWindowHandle = nullptr;

	// 描画機構
	struct DrawingContext;
	std::unique_ptr<DrawingContext> mDrawingContext;
	CComPtr<ID2D1Factory> mD2DFactory;
	drawing::FontLoader mFontLoader;

	// オーディオm関連
	juce::AudioDeviceManager mAudioDeviceManager;
	juce::AudioIODevice* mAudioDevice = nullptr;

	// 再生パラメータ
	std::atomic<float> mPostAmpVolume = 1.0f;

	// シーケンサ,シンセサイザ
	midi::Sequencer mSequencer;
	synth::LuathSynth mSynthesizer;
	
	// 各種ウィジット
	widget::OscilloScope mOscilloScopeWidget;
	widget::SpectrumAnalyzer mSpectrumAnalyzerWidget;
	widget::Lissajous mLissajousWidget;
};

}