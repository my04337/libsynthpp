#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Threading/Task.hpp>
#include <LSP/Threading/EventSignal.hpp>

namespace LSP
{

// タスクディスパッチャ : 各種タスクをワーカースレッドに配給する
class TaskDispatcher final
{
public:
	TaskDispatcher(size_t thread_num = 0);
	~TaskDispatcher();

	// タスク配給停止
	void abort();

	// タスク登録
	void enqueue(std::unique_ptr<Task>&& task, const std::unordered_set<Task::Id>& depends = {});


	// タスク数を取得
	size_t count()const noexcept;
	
private:
	struct TaskInfo final
		: non_copy
	{
		TaskInfo(const std::unordered_set<Task::Id>& depends, std::unique_ptr<Task>&& task) 
			: id(task->id()), depends(depends), task(std::move(task))
		{}

		const Task::Id id;
		std::unordered_set<Task::Id> depends;
		std::unique_ptr<Task> task; //  実行中は空となる
	};
	// タスク取得
	std::unique_ptr<Task> deque();

	// タスク終了通知
	void notifyComplete(Task::Id id);

	// タスクの実行可能性チェック
	bool _executable(const TaskInfo& info)const noexcept;

	// ワーカースレッド メイン関数
	void _threadMain();

private:
	mutable std::mutex mMutex;
	EventSignal mStatusChangedEvent;
	std::atomic_bool mAborted;

	std::list<Task::Id> mWaitingQueue; // 実行前タスク一覧
	std::unordered_map<Task::Id, TaskInfo> mTasks; // タスク一覧(実行中を含む)


	std::unordered_map<std::thread::id, std::unique_ptr<std::thread>> mThreads;
};


}