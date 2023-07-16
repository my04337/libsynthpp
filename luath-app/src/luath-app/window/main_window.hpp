/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <luath-app/window/main_content.hpp>
#include <lsp/synth/synth.hpp>
#include <lsp/midi/sequencer.hpp>

namespace luath::app
{

class MainWindow final
	: public juce::DocumentWindow
	, public juce::FileDragAndDropTarget
	, public juce::AudioIODeviceCallback
	, public juce::MidiInputCallback
	, non_copy_move
{
	using SUPER = juce::DocumentWindow;

public:
	MainWindow();
	~MainWindow()override;

	// Buttons
	void closeButtonPressed()override;
	bool keyPressed(const juce::KeyPress& key)override;

	// Drag & Drop
	bool isInterestedInFileDrag(const juce::StringArray& files)override;
	void filesDropped(const juce::StringArray& files, int x, int y)override;

	// MIDI
	void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)override;

	// Audio
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


private:
	// MIDI関連
	juce::CriticalSection mMidiBufferLock;
	juce::MidiBuffer mMidiBuffer;

	// オーディオ関連
	juce::AudioDeviceManager mAudioDeviceManager;
	juce::AudioIODevice* mAudioDevice = nullptr;

	// 再生パラメータ
	std::atomic<float> mPostAmpVolume = 1.0f;

	// シーケンサ,シンセサイザ
	midi::Sequencer mSequencer;
	synth::LuathSynth mSynthesizer;
	
	// コンポーネント
	std::unique_ptr<MainContent> mMainContent;
};

}