#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>

namespace LSP
{

// タスク : 最小の作業単位(不可分)
class Task
	: non_copy
{
public:
	using Id = issuable_id_base_t<Task>;
	using InterruptionFlag = std::atomic_bool;

public:
	// タスク簡易生成(継承不要)
	template<class F>
	static std::unique_ptr<Task> make(F&& predicate);

	// ---
	virtual ~Task() {}

	// タスクId取得
	Id id()const noexcept { return mId; }
	
	// タスク実行
	virtual void run() = 0;
	
protected:
	Task() : mId(Id::issue()) {}

private:
	const Id mId;
};

// ---

// タスク : 簡単生成用
class FunctionalTask final
	: public Task
{
public:
	using Predicate = std::function<void()>;

public:
	template<class F>
	FunctionalTask(F&& pred) : mPred(std::forward<F>(pred)) {}


	// タスク実行
	virtual void run() { if(mPred) mPred(); }

private:
	Predicate mPred;
};

// ---

template<class F>
static std::unique_ptr<Task> Task::make(F&& pred) { return std::make_unique<FunctionalTask>(std::forward<F>(pred)); }

}

