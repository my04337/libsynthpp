#include <LSP/Stream/MessageQueue.hpp>

using namespace LSP;
using namespace LSP::Stream;

void MessageQueue::push(Message&& msg)
{
	std::lock_guard<decltype(mQueueMutex)> lock(mQueueMutex);

	auto insert_pos = std::find_if(mQueue.cbegin(), mQueue.cend(), [&](const auto& d) { return d > msg; });
	mQueue.emplace(insert_pos, std::move(msg));
}

std::optional<Message> MessageQueue::pop(position_t until)
{
	std::lock_guard<decltype(mQueueMutex)> lock(mQueueMutex);

	if(mQueue.empty()) return {};

	auto& msg = mQueue.front();
	if(msg.position() >= until) return {};
	
	auto ret = std::make_optional(std::move(msg));
	mQueue.pop_front();
	return ret; // NRVO;
}