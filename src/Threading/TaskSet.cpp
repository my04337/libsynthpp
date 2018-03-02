#include <LSP/Threading/TaskSet.hpp>

using namespace LSP;


TaskSet::TaskSet() 
	: mId(TaskSetId::issue()) 
{}
TaskSet::TaskSet(std::vector<std::unique_ptr<ITask>>&& tasks)
	: mId(TaskSetId::issue()) 
{
	for (auto& task : tasks) {
		mTasks.emplace_back(std::move(task));
	}
	tasks.clear();
}
TaskSet::TaskSet(std::list<std::unique_ptr<ITask>>&& tasks)
	: mId(TaskSetId::issue()) 
	, mTasks(std::move(tasks))
{
	tasks.clear();
}

// タスク取得
std::unique_ptr<ITask> TaskSet::get()noexcept
{
	std::lock_guard<decltype(mMutex)> lock(mMutex);

	if(mTasks.empty()) return {};
	std::unique_ptr<ITask> task = std::move(mTasks.front());
	mTasks.pop_front();
	return task; // NRVO
}