#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/SMF/Parser.hpp>


namespace LSP::MIDI::SMF
{


// SMFファイル シーケンサ
class Sequencer
{
public:
	Sequencer(Synthesizer::ToneGenerator& gen);
	~Sequencer();

	// SMFを開きます
	void load(Body&& body);

	// 先頭から再生を開始/再開します
	void start();

	// 再生を停止します
	void stop();

	// 再生中か否かを取得します
	bool isPlaying()const;
	// ---

private:
	void playThreadMain(const Body& messages);

private:
	Synthesizer::ToneGenerator& mToneGenerator;
	std::thread mPlayThread;
	std::atomic_bool mPlayThreadAbortFlag;
	Body mSmfBody;

};

}