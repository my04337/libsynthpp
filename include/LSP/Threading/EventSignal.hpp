#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP
{

/// 通知イベント(Win32API イベントオブジェクト風クラス)
class EventSignal final
	: non_copy_move
{
public:
	EventSignal() : mReady(false), mDispose(false){}

	void dispose();
	void set();
	std::unique_lock<std::mutex> wait();
	std::pair<std::unique_lock<std::mutex>, bool> try_wait();

private:
	// 要求通知イベント
	std::condition_variable mCond;
	std::mutex mMutex;
	bool mReady;
	bool mDispose;
};

}