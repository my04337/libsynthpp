#include <lsp/base/message.hpp>

using namespace lsp;

Message::Message()
	: mPosition(0)
	, mOp(0)
{
}
Message::Message(int64_t pos, uint32_t op, std::any&& param)
	: mPosition(pos)
	, mOp(op)
	, mParam(std::move(param))
{
}
bool Message::operator<(const Message& rhs)const noexcept
{
	return mPosition < rhs.mPosition;
}
bool Message::operator>(const Message& rhs)const noexcept
{
	return mPosition > rhs.mPosition;
}

uint32_t Message::op()const noexcept 
{
	return mOp; 
}

int64_t Message::position()const noexcept 
{
	return mPosition; 
}

const std::any& Message::param()const noexcept 
{ 
	return mParam;
}
std::any& Message::param()noexcept 
{
	return mParam;
}

// ----------------------------------------------------------------------------

void MessageQueue::push(Message&& msg)
{
	std::lock_guard<decltype(mQueueMutex)> lock(mQueueMutex);

	auto insert_pos = std::find_if(mQueue.cbegin(), mQueue.cend(), [&](const auto& d) { return d > msg; });
	mQueue.emplace(insert_pos, std::move(msg));
}

std::optional<Message> MessageQueue::pop(int64_t until)
{
	std::lock_guard<decltype(mQueueMutex)> lock(mQueueMutex);

	if(mQueue.empty()) return {};

	auto& msg = mQueue.front();
	if(msg.position() >= until) return {};

	auto ret = std::make_optional(std::move(msg));
	mQueue.pop_front();
	return ret; // NRVO;
}