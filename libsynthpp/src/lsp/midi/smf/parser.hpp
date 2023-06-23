#pragma once

#include <lsp/base/base.hpp>
#include <lsp/midi/message.hpp>

namespace lsp::midi::smf
{

// SMFファイル 時間形式
enum class TimeUnit {
	TicksPerQuarterNote, // 1時間単位 が 4部音符の何分の1の長さか (例:TicksPerQuarterNote=120の場合、4分音符は480, 8分音符は60
};

// SMFヘッダ
struct Header
{
	// SMF フォーマット
	uint16_t format;

	// トラック数
	uint16_t trackNum;

	// タイムベース(4部音符当たりの分解能)
	uint16_t ticksPerQuarterNote;
};

using Body = std::vector<std::pair<std::chrono::microseconds, std::shared_ptr<const Message>>>;

// SMF解析エラー
class decoding_exception 
	: public std::runtime_error 
{
	using runtime_error::runtime_error;
};


// SMFファイル パーサ
class Parser
	: non_copy_move
{
public:
	static std::pair<Header, Body> parse(const std::filesystem::path& path); // throws decoding_exception

protected:
	Parser(std::istream& s) : s(s) {}

	void stageHeader(Header& header);
	void stageTrack(std::vector<std::pair<uint64_t, std::unique_ptr<Message>>>& messages);

private:
	std::istream& s;
};

}