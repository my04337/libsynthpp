#include <LSP/Threading/TaskDispatcher.hpp>

#include <LSP/Base/Logging.hpp>

using namespace LSP;

TaskDispatcher::TaskDispatcher(size_t thread_num)
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

void TaskDispatcher::enqueue(std::unique_ptr<Task>&& task, const std::unordered_set<Task::Id>& depends)
{
	std::unique_lock<decltype(mMutex)> lock(mMutex);

	// タスクを登録
	TaskInfo info(depends, std::move(task));
	auto id = info.id;
	mTasks.emplace(id, std::move(info));
	mWaitingQueue.push_back(id);
	lock.unlock();

	// 状態変更を通知
	mStatusChangedEvent.set();
}

std::unique_ptr<Task> TaskDispatcher::deque()
{
	while (true) {
		std::unique_lock<decltype(mMutex)> lock(mMutex);

		// 処理可能なタスクを探す
		for (auto iter = mWaitingQueue.begin(); iter != mWaitingQueue.end(); ++iter) {
			auto id = *iter;
			auto found = mTasks.find(id);
			lsp_assert(found != mTasks.end());
			auto& info = found->second;
			if(!_executable(info)) continue;

			// 処理可能タスクを発見 : 呼び元に渡す
			lsp_assert(info.task != nullptr);
			mWaitingQueue.erase(iter);
			return std::move(info.task);
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
// タスク終了通知 (for Thread)
void TaskDispatcher::notifyComplete(Task::Id id)
{
	std::unique_lock<decltype(mMutex)> lock(mMutex);

	// 終了済みタスクセットを削除
	auto found = mTasks.find(id);
	lsp_assert(found != mTasks.end());
	TaskInfo& info = found->second;
	lsp_assert(info.task == nullptr);
	mTasks.erase(found);

	// 状態変更を通知
	lock.unlock();
	mStatusChangedEvent.set();
}

bool TaskDispatcher::_executable(const TaskInfo& info)const noexcept
{
	for (auto depend : info.depends) {
		if(mTasks.find(depend) != mTasks.end()) return false;
	}
	return true;
}
void TaskDispatcher::_threadMain()
{
	while (true) {
		if(mAborted) break;

		// 処理すべきタスクセットを取得
		auto task = deque();

		if(!task) continue;

		// タスク実行
		task->run();
		
		// 終了通知
		notifyComplete(task->id());
	}
}