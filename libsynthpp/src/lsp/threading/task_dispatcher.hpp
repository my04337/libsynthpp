#pragma once

#include <lsp/base/base.hpp>
#include <lsp/threading/thread.hpp>
#include <lsp/threading/event_signal.hpp>

#include <future>

namespace LSP::Threading
{
// タスクディスパッチャ : 各種タスクをワーカースレッドに配給する
class TaskDispatcher final
{
public:
	TaskDispatcher(size_t thread_num = 0, Priority priority = Priority::Inherited);
	~TaskDispatcher();

	// タスク配給停止
	void abort();

	// タスク登録
	template<class Predicate, class=std::enable_if_t<std::is_invocable_v<Predicate>>>
	[[nodiscard]]
	std::future<std::invoke_result_t<Predicate>> enqueue(Predicate pred);

	// タスク数を取得
	size_t count()const noexcept;
	
private:
	// タスク取得
	std::function<void()> deque();
	
	// ワーカースレッド メイン関数
	void _threadMain();

private:
	mutable std::mutex mMutex;
	EventSignal mStatusChangedEvent;
	std::atomic_bool mAborted;

	std::list<std::function<void()>> mWaitingQueue; // 実行前タスク一覧


	std::unordered_map<std::thread::id, std::unique_ptr<std::thread>> mThreads;
};

// ---

template<class Predicate, class>
std::future<std::invoke_result_t<Predicate>> TaskDispatcher::enqueue(Predicate pred_)
{
	using ReturnType = typename std::invoke_result_t<Predicate>;
	auto p = std::make_shared<std::promise<ReturnType>>();
	std::future<ReturnType> f = p->get_future();

	auto pred = std::make_shared<Predicate>(std::move(pred_));

	auto task = [p = std::move(p), pred = std::move(pred)]() {
		try {
			p->set_value((*pred)());
		} catch (...) {
			p->set_exception(std::current_exception());
		}
	};

	{
		std::lock_guard lock(mMutex);
		mWaitingQueue.emplace_back(std::move(task));
	}
	mStatusChangedEvent.set();


	return f;
}

}