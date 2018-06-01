#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>

namespace LSP::MIDI::Messages
{

/// ソングポジション
class SongPosition
	: public Message
{
public:
	SongPosition(uint16_t pos)
		: mSongPos(pos)
	{}

private:
	uint16_t mSongPos;
};

/// ソングセレクト
class SongSelect
	: public Message
{
public:
	SongSelect(uint8_t songId)
		: mSongId(songId)
	{}

private:
	uint8_t mSongId;
};

/// チューンリクエスト
class TuneRequest
	: public Message
{
public:
};

/// タイミングクロック
class TimingClock
	: public Message
{
public:
};

/// スタート
class Start
	: public Message
{
public:
};

/// コンティニュー
class Continue
	: public Message
{
public:
};

/// ストップ
class Stop
	: public Message
{
public:
};

/// アクティブセンシング
class ActiveSensing
	: public Message
{
public:
};


}