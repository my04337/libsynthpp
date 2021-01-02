#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>

namespace LSP::MIDI
{
// MIDIメッセージ レシーバ
class MessageReceiver
	: non_copy_move
{
public:
	virtual ~MessageReceiver() {}

	// MIDIメッセージ受信コールバック : メッセージ類は蓄積される
	virtual void onMidiMessageReceived(clock::time_point msg_time, const std::shared_ptr<const Message>& msg) = 0;
};

}