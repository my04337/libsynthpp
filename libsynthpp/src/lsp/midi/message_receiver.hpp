#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/message.hpp>

namespace lsp::midi
{
// MIDI���b�Z�[�W ���V�[�o
class MessageReceiver
	: non_copy_move
{
public:
	virtual ~MessageReceiver() {}

	// MIDI���b�Z�[�W��M�R�[���o�b�N : ���b�Z�[�W�ނ͒~�ς����
	virtual void onMidiMessageReceived(clock::time_point msg_time, const std::shared_ptr<const Message>& msg) = 0;
};

}