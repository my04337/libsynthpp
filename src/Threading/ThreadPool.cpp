#include <LSP/Threading/ThreadPool.hpp>
#include <LSP/Threading/TaskDispatcher.hpp>

using namespace LSP;

ThreadPool::ThreadPool(TaskDispatcher& dispatcher, size_t thread_num)
	: mDispatcher(dispatcher)
	, mAborted(false)
{
	// スレッド数が指定されなかった場合、自動的にスレッド数を決める
	if (thread_num == 0) {
		thread_num = std::thread::hardware_concurrency();
		if (thread_num == 0) {
			thread_num = 1;
		}
	}

	// スレッド生成
	for(size_t i=0; i<thread_num; ++i) {
		auto th = std::make_unique<std::thread>([this]{threadMain();});
		auto thId = th->get_id();
		mThreads.emplace(thId, std::move(th));
	}
}
ThreadPool::~ThreadPool()
{
	// 全スレッドを終了 (処理中の仕事は速やかに中断)
	mAborted = true;
	for (auto& kvp : mThreads) {
		if (kvp.second->joinable()) {
			kvp.second->join();
		}
		kvp.second.reset();
	}
}

void ThreadPool::threadMain()
{
	while (true) {
		if(mAborted) break;

		auto task = mDispatcher.deque();
		if(!task) continue;

		task();
	}
}