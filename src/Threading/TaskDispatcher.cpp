#include <LSP/Threading/TaskDispatcher.hpp>

#include <LSP/Debugging/Logging.hpp>

using namespace LSP;

TaskDispatcher::TaskDispatcher()
{

}
TaskDispatcher::~TaskDispatcher()
{
	abort();
}

void TaskDispatcher::abort()
{
	mAborted = true;
	mStatusChangedEvent.dispose();
}

void TaskDispatcher::enqueue(Task&& task, const std::unordered_set<TaskId>& depends)
{
	if(!task) return; // 空タスクは無視

	std::lock_guard<decltype(mMutex)> lock(mMutex);

	mTasks.emplace_back(std::move(task));
	mStatusChangedEvent.set();
}

Task TaskDispatcher::deque()
{
	while (true) {
		std::unique_lock<decltype(mMutex)> lock(mMutex);

		// タスクがある場合は直ぐに返す
		if (!mTasks.empty()) {
			Task task = std::move(mTasks.front());
			mTasks.pop_front();
			return task; // NRVO
		}

		// 無ければタスクが供給されるまで待機
		lock.unlock();
		mStatusChangedEvent.wait();

		// 待機終了後、abortされていれば中断,そうでなければリトライ
		if(mAborted) return {};
	}
}
// タスク数を取得
size_t TaskDispatcher::count()const noexcept
{
	std::lock_guard<decltype(mMutex)> lock(mMutex);

	return mTasks.size();
}