/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/message_receiver.hpp>
#include <lsp/midi/smf/parser.hpp>


namespace lsp::midi::smf
{
// SMFファイル シーケンサ
class Sequencer
	: non_copy_move
{
public:
	Sequencer(MessageReceiver& receiver);
	~Sequencer();

	// SMFを開きます
	void load(Body&& body);

	// 先頭から再生を開始/再開します
	void start();

	// 再生を停止します
	void stop();

	// 再生中か否かを取得します
	bool isPlaying()const;

	// システムリセットを送信します
	void reset(SystemType type);

private:
	void playThreadMain(const Body& messages);

private:
	MessageReceiver& mReceiver;
	std::thread mPlayThread;
	std::atomic_bool mPlayThreadAbortFlag;
	Body mSmfBody;

};

}