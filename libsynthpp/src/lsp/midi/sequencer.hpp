/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>


namespace lsp::midi
{
// SMFファイル シーケンサ
class Sequencer
	: non_copy_move
{
public:
	Sequencer(juce::MidiInputCallback& receiver);
	~Sequencer();

	// SMFを開きます
	void load(juce::MidiFile&& midiFile);

	// 先頭から再生を開始/再開します
	void start();

	// 再生を停止します
	void stop();

	// 再生中か否かを取得します
	bool isPlaying()const;

private:
	void playThreadMain(std::stop_token stopToken, const juce::MidiMessageSequence& messages);

private:
	juce:: MidiInputCallback& mReceiver;
	std::jthread mPlayThread;
	juce::MidiMessageSequence mSequence;

};

}