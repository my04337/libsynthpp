#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Threading/Task.hpp>

namespace LSP
{

// タスクセットId
struct _task_set_id_tag {};
using TaskSetId = issuable_id_base_t<_task_set_id_tag>;

// タスク : タスクを束ねたもの (可分,基本的に分割せずに処理するのが好ましい)
class TaskSet final
	: non_copy
{
public:
	TaskSet();
	TaskSet(std::vector<std::unique_ptr<ITask>>&& tasks);
	TaskSet(std::list<std::unique_ptr<ITask>>&& tasks);

	// タスクセットId取得
	TaskSetId id()const noexcept { return mId; }

	// タスク取得 (スレッドセーフ)
	std::unique_ptr<ITask> get()noexcept;

private:
	const TaskSetId mId;
	mutable std::mutex mMutex;
	std::list<std::unique_ptr<ITask>> mTasks;
};

}