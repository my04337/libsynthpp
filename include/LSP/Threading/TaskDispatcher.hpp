#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Threading/Task.hpp>

namespace LSP
{

// タスクディスパッチャ : 各種タスクをワーカースレッドに配給する
class TaskDispatcher
{
public:
	// タスク登録 (注意 : 
	template<class InputIterator>
	void enqueue(InputIterator begin, InputIterator end, const std::vector<TaskId>& depends_first = {});

	void enqueue(Task&& task, const std::vector<TaskId>& depends = {});

	
private:

private:
	std::mutex mMutex;
	std::unordered_map<TaskId, Task> mTasks;
};

// ----------------------------------------------------------------------------

template<class InputIterator>
void TaskDispatcher::enqueue(InputIterator first, InputIterator last, const std::vector<TaskId>& depends_first)
{
	auto iter = first;
	if(iter == last) return;

	// 最初のタスクを登録
	std::vector<TaskId> depend {iter->id()};
	enqueue(std::move(*iter), depends_first); 
	++iter;

	// 二個目以降のタスクを登録
	for (; iter != last; ++iter) {
		TaskId id = iter->id();
		enqueue(std::move(*iter), depend);
		depend[0] = id;
	}
}

}