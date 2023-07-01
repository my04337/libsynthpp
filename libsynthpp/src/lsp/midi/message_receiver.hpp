/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/message.hpp>

namespace lsp::midi
{
// MIDIメッセージ レシーバ
class MessageReceiver
	: non_copy_move
{
public:
	virtual ~MessageReceiver() {}

	// MIDIメッセージ受信コールバック : メッセージ類は蓄積される
	virtual void onMidiMessageReceived(clock::time_point msg_time, const juce::MidiMessage& msg) = 0;
};

}