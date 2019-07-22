#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>

namespace LSP::MIDI::Messages
{

/// MIDI メタイベント 基底クラス
class MetaEvent
	: public Message
{
public:	
};

// セットテンポ
class SetTempo
	: public MetaEvent
{
public:
	SetTempo(std::chrono::microseconds timePerQuarterNote)
		: mTimePerQuarterNote(timePerQuarterNote)
	{}

	std::chrono::microseconds timePerQuarterNote()const noexcept { return mTimePerQuarterNote; }

private:
	std::chrono::microseconds mTimePerQuarterNote;
};


/// MIDI メタイベント (汎用)
class GeneralMetaEvent
	: public MetaEvent
{
public:
	GeneralMetaEvent(uint8_t type)
		: mType(type)
	{}

	uint8_t type()const noexcept { return mType; } 

private:
	uint8_t mType;
};

}