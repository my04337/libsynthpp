#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>

namespace LSP
{

// タスクId
struct _task_id_tag {};
using TaskId = issuable_id_base_t<_task_id_tag>;


// タスク : 最小の作業単位(不可分)
class ITask
	: non_copy_move
{
public:
	ITask() : mId(TaskId::issue()) {}
	virtual ~ITask() {}

	// タスクId取得
	TaskId id()const noexcept { return mId; }

	// タスク実行
	virtual void run() = 0;
	void operator()() { run(); }

private:
	TaskId mId;
};

// タスク実装 : 汎用版 - ファンクタやラムダ式等に対応
class Task
	: public ITask
{
public:
	using Predicate = std::function<void()>;

public:
	template<class F>
	Task(F&& pred) : mPred(std::forward<F>(pred)) {}
	
	// タスク実行
	virtual void run()override { mPred(); }

private:
	Predicate mPred;
};

}

