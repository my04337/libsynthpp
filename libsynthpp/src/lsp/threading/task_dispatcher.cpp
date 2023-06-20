#include <LSP/Threading/TaskDispatcher.hpp>

#include <LSP/Base/Logging.hpp>

using namespace LSP::Threading;

TaskDispatcher::TaskDispatcher(size_t thread_num,  Priority priority)
	: mAborted(false)
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
		auto th = std::make_unique<std::thread>([this]{_threadMain();});
		auto thId = th->get_id();
		setThreadPriority(*th, priority);
		mThreads.emplace(thId, std::move(th));
	}
}
TaskDispatcher::~TaskDispatcher()
{
	// 先に停止指示
	abort();

	// 全スレッドを終了 (処理中の仕事は速やかに中断)
	for (auto& kvp : mThreads) {
		if (kvp.second->joinable()) {
			kvp.second->join();
		}
		kvp.second.reset();
	}
}

void TaskDispatcher::abort()
{
	mAborted = true;
	mStatusChangedEvent.dispose();
}


std::function<void()> TaskDispatcher::deque()
{
	while (true) {
		std::unique_lock<decltype(mMutex)> lock(mMutex);

		// 処理可能なタスクを探す
		if (!mWaitingQueue.empty()) {
			// 処理可能タスクを発見 : 呼び元に渡す
			auto task = std::move(mWaitingQueue.front());
			mWaitingQueue.pop_front();

			Assertion::check(task != nullptr);
			return task; // NRVO
		}

		// 無ければタスクが供給されるまで待機
		lock.unlock();
		mStatusChangedEvent.wait(EventSignal::NoLock);

		// 待機終了後、abortされていれば中断,そうでなければリトライ
		if(mAborted) return {};
	}
}
// タスク数を取得
size_t TaskDispatcher::count()const noexcept
{
	std::lock_guard<decltype(mMutex)> lock(mMutex);

	return mWaitingQueue.size();
}

void TaskDispatcher::_threadMain()
{
	while (true) {
		if(mAborted) break;

		// 処理すべきタスクセットを取得
		auto task = deque();

		if(!task) continue;

		// タスク実行
		task();
	}
}