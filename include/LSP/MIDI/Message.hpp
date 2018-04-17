#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::MIDI
{
class Sequencer;

/// MIDIメッセージ  抽象クラス
class Message
{
public:
	virtual ~Message() {}

	// メッセージを処理します
	virtual void play(Sequencer& seq)const = 0;
};

}
