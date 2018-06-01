#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::MIDI
{

/// MIDIメッセージ  抽象クラス
class Message
{
public:
	virtual ~Message() {}

	// 対象チャネルを取得します (0xff=非チャネルボイスメッセージ
	virtual uint8_t channel()const noexcept { return std::numeric_limits<uint8_t>::max(); }
};

// チャネルボイスメッセージ : 特定チャネル向けメッセージ
class ChannelVoiceMessage
	: public Message
{
public:
	// 対象チャネルを取得します
	virtual uint8_t channel()const noexcept = 0;
};

}
