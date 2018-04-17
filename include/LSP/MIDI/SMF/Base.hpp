#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::MIDI::SMF
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

// SMF解析エラー
class decoding_exception 
	: public std::runtime_error 
{
	using runtime_error::runtime_error;
};

}