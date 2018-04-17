#pragma once

#include <LSP/MIDI/SMF/Base.hpp>

namespace LSP::MIDI { class Message; class Sequencer; }

namespace LSP::MIDI::SMF
{


// SMFファイル プレイヤー
class Player
{
public:
	Player(Sequencer& seq);
	~Player();

	// SMFを開きます
	void open(const std::filesystem::path& path); // throws decoding_exception

	// 先頭から再生を開始/再開します
	void start();

	// 再生を停止します
	void stop();

	// 再生中か否かを取得します
	bool isPlaying()const;
	// ---

private:
	void stageHeader(std::istream& s, Header& header);
	void stageTrack(std::istream& s, std::vector<std::pair<uint64_t, std::unique_ptr<Message>>>& messages);

	void playThreadMain(const std::vector<std::pair<std::chrono::microseconds, std::shared_ptr<const Message>>>& messages);

private:
	Sequencer& mSeq;
	std::thread mPlayThread;
	std::atomic_bool mPlayThreadAbortFlag;
	std::vector<std::pair<std::chrono::microseconds, std::shared_ptr<const Message>>> mMessages;

};

}