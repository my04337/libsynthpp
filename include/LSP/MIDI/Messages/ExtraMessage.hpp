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

	virtual void play(Sequencer& seq)const override {} // TODO 未実装

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

	virtual void play(Sequencer& seq)const override {} // TODO 未実装

private:
	uint8_t mSongId;
};

/// チューンリクエスト
class TuneRequest
	: public Message
{
public:
	virtual void play(Sequencer& seq)const override {} // TODO 未実装
};

/// タイミングクロック
class TimingClock
	: public Message
{
public:
	virtual void play(Sequencer& seq)const override {} // TODO 未実装
};

/// スタート
class Start
	: public Message
{
public:
	virtual void play(Sequencer& seq)const override {} // TODO 未実装
};

/// コンティニュー
class Continue
	: public Message
{
public:
	virtual void play(Sequencer& seq)const override {} // TODO 未実装
};

/// ストップ
class Stop
	: public Message
{
public:
	virtual void play(Sequencer& seq)const override {} // TODO 未実装
};

/// アクティブセンシング
class ActiveSensing
	: public Message
{
public:
	virtual void play(Sequencer& seq)const override {} // TODO 未実装
};


}