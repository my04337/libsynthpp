#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/message.hpp>

namespace lsp::midi::messages
{

/// MIDI システムエクスクルーシブメッセージ
class SysExMessage
	: public Message
{
public:
	SysExMessage(std::vector<uint8_t>&& data)
		: mData(std::move(data))
	{}

	const std::vector<uint8_t>& data()const noexcept { return mData; }

private:
	std::vector<uint8_t> mData;
};

}