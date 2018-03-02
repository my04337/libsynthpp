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
	TaskDispatcher();
	~TaskDispatcher();

	// タスク配給停止
	void abort();

	// タスク登録
	template<class InputIterator>
	void enqueue(InputIterator begin, InputIterator end, const std::unordered_set<TaskId>& depends_first = {});
	void enqueue(Task&& task, const std::unordered_set<TaskId>& depends = {});

	// タスク取得
	Task deque();

	// タスク数を取得
	size_t count()const noexcept;
	
private:

private:
	mutable std::mutex mMutex;
	EventSignal mStatusChangedEvent;
	std::atomic_bool mAborted;

	std::deque<Task> mTasks; // TODO 依存関係考慮
};

// ----------------------------------------------------------------------------

template<class InputIterator>
void TaskDispatcher::enqueue(InputIterator first, InputIterator last, const std::unordered_set<TaskId>& depends_first)
{
	auto iter = first;
	if(iter == last) return;

	// 最初のタスクを登録
	std::unordered_set<TaskId> depend {iter->id()};
	enqueue(std::move(*iter), depends_first); 
	++iter;

	// 二個目以降のタスクを登録
	for (; iter != last; ++iter) {
		TaskId id = iter->id();
		enqueue(std::move(*iter), depend);

		depend.clear();
		depend.insert(id);
	}
}

}