/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/window/main_window.hpp>
#include <luath-app/app/application.hpp>

#include <array>
#include <map>
#include <numeric>

using namespace luath::app;

MainWindow::MainWindow()
	: juce::DocumentWindow(
		juce::JUCEApplication::getInstance()->getApplicationName(),
		juce::Colour::fromRGB(0xFF, 0xFF, 0xFF), 
		juce::DocumentWindow::TitleBarButtons::minimiseButton | juce::DocumentWindow::TitleBarButtons::closeButton
	)
	, mSequencer(*this)
	, mSynthesizer()
{
	using namespace std::string_literals;

	// ウィンドウ周りセットアップ
	mMainContent = std::make_unique<MainContent>(mSynthesizer, mAudioDeviceManager);
	setUsingNativeTitleBar(true);
	setContentOwned(mMainContent.get(), true);
	centreWithSize(getWidth(), getHeight());

	// シーケンサセットアップ
	auto midi_path = std::filesystem::current_path();
	midi_path.append(L"assets/sample_midi/brambles_vsc3.mid"s); // 試験用MIDIファイル
	loadMidi(midi_path);

	// オーディオ周り初期化
	mAudioDeviceManager.initialiseWithDefaultDevices(
		0, // numInputChannelsNeeded
		2 // numOutputChannelsNeeded
	);
	mAudioDevice = mAudioDeviceManager.getCurrentAudioDevice();
	if(!mAudioDevice) {
		Log::w("Audio device is not found.");
	}
	mAudioDeviceManager.addAudioCallback(this);
}

MainWindow::~MainWindow()
{
	// 再生停止
	mAudioDeviceManager.closeAudioDevice();
	mAudioDeviceManager.removeAudioCallback(this);

	// シーケンサ停止
	mSequencer.stop();

	// トーンジェネレータ停止
	mSynthesizer.dispose();
}

void MainWindow::closeButtonPressed()
{
	juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

bool MainWindow::keyPressed(const juce::KeyPress& key)
{
	if (key == juce::KeyPress::upKey) {
		mPostAmpVolume.store(mPostAmpVolume.load() * 1.5f);
		return true;
	} else if(key == juce::KeyPress::downKey) {
		mPostAmpVolume.store(mPostAmpVolume.load() / 1.5f);
		return true;
	}
	return false;
}
bool MainWindow::isInterestedInFileDrag(const juce::StringArray& files)
{
	return files.size() == 1;
}
void MainWindow::filesDropped(const juce::StringArray& files, int x, int y)
{
	auto midi_path = std::filesystem::path(files[0].toWideCharPointer());
	loadMidi(midi_path);
}

void MainWindow::loadMidi(const std::filesystem::path& path) {
	// 現在の再生を停止
	mSequencer.stop();

	// MIDIファイルをロード
	juce::FileInputStream midiInputStream(juce::File(path.wstring().c_str()));
	juce::MidiFile midiFile;
	if(!midiFile.readFrom(midiInputStream)) {
		// ロード失敗
		Log::e("Broken midi file : {}", path.string());
	}
	
	// 再生開始
	mSequencer.load(std::move(midiFile));
	mSequencer.start();
}

void MainWindow::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
	juce::ScopedLock sl(mMidiBufferLock);

	// TODO 本来 sampleNumber は次にレンダリングするMIDIメッセージが何サンプル目に解釈すべきかを示しているべき。
	//      smd::midi::Sequencer 側のあり方含めて検討すること
	int sampleNumber = 0;
	mMidiBuffer.addEvent(message, sampleNumber);
}

void MainWindow::audioDeviceIOCallbackWithContext(
	const float* const* inputChannelData,
	int numInputChannels,
	float* const* outputChannelData,
	int numOutputChannels,
	int numSamples,
	const juce::AudioIODeviceCallbackContext& context
)
{
	// 出力先の準備
	check(numOutputChannels == 2);
	check(numSamples >= 0);
	auto sig = lsp::Signal<float>::fromAudioBuffer(
		juce::AudioBuffer<float>(
			outputChannelData,	// 出力先バッファに直接書き込むようにする
			numOutputChannels,
			numSamples
		)
	);

	// シンセサイザ発音
	{
		juce::ScopedLock sl(mMidiBufferLock);
		mSynthesizer.renderNextBlock(sig.data(), mMidiBuffer, 0, numSamples);
		mMidiBuffer.clear();
	}

	// ポストエフェクト適用
	auto postAmpVolume = mPostAmpVolume.load();
	for(uint32_t ch = 0; ch < sig.channels(); ++ch) {
		auto data = sig.mutableData(ch);
		for(size_t i = 0; i < sig.samples(); ++i) {
			data[i] *= postAmpVolume;
		}
	}
	

	// UI側へ配送
	// TODO 信号生成に影響を与えにくい経路で配送したい
	mMainContent->writeAudio(sig);
}
void MainWindow::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
	mAudioDevice = device;
	mMainContent->audioDeviceAboutToStart(device);

	if(device) {
		auto sampleFreq = static_cast<float>(device->getCurrentSampleRate());
		mSynthesizer.setCurrentPlaybackSampleRate(device->getCurrentSampleRate());
	}
}
void MainWindow::audioDeviceStopped()
{
	// nop
}
void MainWindow::audioDeviceError(const juce::String& errorMessage)
{
	Log::e("Audio device error : {}", errorMessage.toStdString());
}
