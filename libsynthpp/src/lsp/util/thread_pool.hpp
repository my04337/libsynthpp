﻿// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/core/logging.hpp>
#include <lsp/util/thread_priority.hpp>

namespace lsp
{

class ThreadPool
{
public:
    using Task = std::function<void()>;

    ThreadPool(size_t numThreads, std::optional<ThreadPriority> priority = std::nullopt, std::function<void()> onStart = {}, std::function<void()>onEnd = {});
    ~ThreadPool();

    // タスクを追加します
    template <typename... Funcs>
    auto enqueue(Funcs&&... funcs) {
        if constexpr(sizeof...(Funcs) == 1) {
            // 単一の関数の場合 : std::future を返します
            using FuncType = std::tuple_element_t<0, std::tuple<Funcs...>>;
            using ReturnType = decltype(std::declval<FuncType>()());

            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::forward<FuncType>(std::get<0>(std::make_tuple(funcs...)))
            );

            std::future<ReturnType> res = task->get_future();
            {
                std::lock_guard lock(_mutex);
                _tasks.emplace_back([task]() { (*task)(); });
            }
            _tasksSemaphore.release();
            return res;
        }
        else {
            // 複数の関数の場合 : std::array<std::future<...>> を返します
            auto taskTuple = std::make_tuple(std::forward<Funcs>(funcs)...);
            auto sequence = std::make_index_sequence<sizeof...(Funcs)>{};
            return enqueueHelper(sequence, taskTuple);
        }
    }

private:
    // ヘルパー関数テンプレート
    template <typename Func, size_t... I>
    auto enqueueHelper(std::index_sequence<I...>, Func&& func) {
        return std::make_tuple(enqueue(std::get<I>(func))...);
    }


private:
    // ワーカースレッド
    std::vector<std::jthread> _workers;

    // タスクキュー用メモリプール
    std::pmr::synchronized_pool_resource _mem;

    // 排他用ミューテックス
    std::mutex _mutex;

    // タスクキュー
    std::pmr::deque<Task> _tasks;

    // タスクキュー追加通知用セマフォ
    std::counting_semaphore<> _tasksSemaphore;
};

}