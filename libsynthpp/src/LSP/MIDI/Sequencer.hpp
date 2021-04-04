#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Parser.hpp>


namespace LSP::MIDI
{
class MessageReceiver;

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