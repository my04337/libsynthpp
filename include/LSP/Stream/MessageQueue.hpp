#pragma once

#include <LSP/Stream/StreamBase.hpp>
#include <LSP/Stream/Message.hpp>

namespace LSP::Stream
{

// 制御メッセージ キュー
class MessageQueue final
	: non_copy
{
public:

	// メッセージをキューイングします
	void push(Message&& msg);

	// キューイングされたメッセージを取得します (ただし時刻がuntilより前のものに限る)
	std::optional<Message> pop(position_t until = MAX_POSITION);

private:
	mutable std::mutex mQueueMutex;
	std::deque<Message> mQueue;
};

}