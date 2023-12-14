/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/util/thread_pool.hpp>

using namespace lsp;


ThreadPool::ThreadPool(size_t numThreads, std::function<void()> onStart, std::function<void()>onEnd)
    : _tasks(std::pmr::get_default_resource())
    , _tasksSemaphore(0)
{
    require(numThreads > 0, "numThreads must be greater than 0");

    // 必要数のスレッドを生成
    for(size_t i = 0; i < numThreads; ++i) {
        _workers.emplace_back([this, onStart, onEnd](std::stop_token stopToken) {
            // スレッド開始コールバック
            if(onStart) onStart();

            // タスクを受け取り処理するループ
            while(true) {
                // タスクを一つ要求
                _tasksSemaphore.acquire();

                // スレッド停止要求があればすみやかに終了する
                if(stopToken.stop_requested()) break;

                // タスクの取り出し
                Task task;
                {
                    std::lock_guard lock(_mutex);
                    
                    if(_tasks.empty()) continue;

                    task = std::move(_tasks.front());
                    _tasks.pop_front();
                }

                // タスクの実行
                task();
            }

            // スレッド終了コールバック
            if(onEnd) onEnd();
        });
    }
}

ThreadPool::~ThreadPool()
{
    // 各スレッドにdisposeされたことを通知 ※これ以降追加は不可
    for(auto& worker : _workers) {
        worker.request_stop();
    }

    // 全スレッドが起き上がれるようにスレッド数分だけセマフォを上げる
    _tasksSemaphore.release(_workers.size());

    // スレッドの終了待機はデストラクタで暗黙的に行われるため、ここでは行わない
}