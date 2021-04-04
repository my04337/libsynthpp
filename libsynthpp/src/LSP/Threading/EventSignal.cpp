#include <LSP/Threading/EventSignal.hpp>

using namespace LSP::Threading;


void EventSignal::dispose()
{
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mReady = true;
		mDispose = true;
	}
	mCond.notify_all();
}

void EventSignal::set()
{
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mReady = true;
	}
	mCond.notify_one();
}
std::unique_lock<std::mutex> EventSignal::wait()
{
	std::unique_lock<std::mutex> lock(mMutex);
	mCond.wait(lock, [this]()->bool{return mReady|mDispose;});
	mReady = false;
	return lock;
}
std::pair<std::unique_lock<std::mutex>, bool> EventSignal::try_wait()
{
	std::unique_lock<std::mutex> lock(mMutex);
	bool signaled = mCond.wait_for(lock, std::chrono::milliseconds(0), [this]()->bool{return mReady|mDispose;});
	mReady = false;
	return std::make_pair(std::move(lock), signaled);
}
