#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::MIDI::SMF
{

enum class TimeUnit {
	TicksPerFrame,
};

// SMFヘッダ
struct Header
{
	// SMF フォーマット
	uint16_t format;

	// トラック数
	uint16_t trackNum;

	// タイムベース(4部音符当たりの分解能)
	uint16_t ticksPerFrame;
};


// SMFファイルパーサ
class Parser
{
	class parsing_exception : public std::runtime_error {
		using runtime_error::runtime_error;
	};
public:
	Parser();

	// SMFを解析します
	void parse(std::istream& s); // throws parsing_exception

	// ---

	// 解析済みヘッダ情報を取得します
	const Header& header()const noexcept { return mHeader; }  

private:
	void stageHeader(std::istream& s);
	void stageTrack(std::istream& s);

private:
	Header mHeader;
};

}