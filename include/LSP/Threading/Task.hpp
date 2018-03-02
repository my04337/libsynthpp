#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>

namespace LSP
{

// タスクId
struct _task_id_tag {};
using TaskId = issuable_id_base_t<_task_id_tag>;


// タスク : 最小の作業単位(不可分)
class Task final
	: non_copy
{
	using Predicate = std::function<void()>;

public:
	// タスク簡易生成(継承不要)
	template<class F>
	static Task make(F&& pred) { return Task(std::forward<F>(pred)); }

	// ---

	Task() : mId() {}
	template<class F>
	Task(F&& pred) : mId(TaskId::issue()) , mPred(std::forward<F>(pred)) {}

	// タスクId取得
	TaskId id()const noexcept { return mId; }

	// タスクが有効か否かを取得
	bool empty()const noexcept{ return mId.empty(); }
	operator bool()const noexcept { return !empty(); }

	// タスク実行 (一回のみ可能)
	void run() { if(mPred) { mPred(); mPred = nullptr; } }
	void operator()() { run(); }
	
private:
	const TaskId mId;
	Predicate mPred;
};

}

