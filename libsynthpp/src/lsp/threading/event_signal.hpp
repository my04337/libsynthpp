#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::Threading
{

/// 通知イベント(Win32API イベントオブジェクト風クラス)
class EventSignal final
	: non_copy_move
{
public:
	struct NoLockPolicy{};
	static inline NoLockPolicy NoLock{};

public:
	EventSignal() : mReady(false), mDispose(false){}

	void dispose();
	void set();

	[[nodiscard]] std::unique_lock<std::mutex> wait();
	void wait(NoLockPolicy);

	[[nodiscard]] std::pair<std::unique_lock<std::mutex>, bool> try_wait();
	bool try_wait(NoLockPolicy);

private:
	// 要求通知イベント
	std::condition_variable mCond;
	std::mutex mMutex;
	bool mReady;
	bool mDispose;
};

}