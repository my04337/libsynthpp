// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

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
	void load(const std::filesystem::path& path);

	// 先頭から再生を開始/再開します
	void start();

	// 再生を停止します
	void stop();

	// 再生中か否かを取得します
	bool isPlaying()const;

private:
	void playThreadMain(std::stop_token stopToken, const smf::MidiFile& parsedMidiFile);

private:
	juce:: MidiInputCallback& mReceiver;
	std::jthread mPlayThread;
	std::unique_ptr<const smf::MidiFile> mParsedMidiFile;

};

}