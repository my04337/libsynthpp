#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>

namespace LSP::MIDI::Messages
{

/// MIDI システムエクスクルーシブメッセージ
class SysExMessage
	: public Message
{
public:
	SysExMessage(std::vector<uint8_t>&& data)
		: mData(std::move(data))
	{}

private:
	std::vector<uint8_t> mData;
};

}