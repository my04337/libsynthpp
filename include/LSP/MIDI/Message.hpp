#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::MIDI::Synthesizer { class ToneGenerator; }

namespace LSP::MIDI
{

/// MIDIメッセージ  抽象クラス
class Message
{
public:
	virtual ~Message() {}

	// メッセージを処理します
	virtual void play(Synthesizer::ToneGenerator& gen)const = 0;
};

}
