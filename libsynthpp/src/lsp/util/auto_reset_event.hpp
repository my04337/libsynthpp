// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIThttps://opensource.org/license/mit/

#pragma once

#include <lsp/core/core.hpp>

namespace lsp
{

// .NET FrameworkのAutoResetEventクラスのC++版
class AutoResetEvent {
public:
    explicit AutoResetEvent(bool initial = false)
        : _flag(initial), _semaphore(initial ? 1 : 0)
    {}

    // イベントをシグナル状態にします
    void set()
    {
        if(!_flag.exchange(true)) {
            _semaphore.release();
        }
    }

    // イベントのシグナル状態をリセットします
    void reset()
    {
        _flag = false;
    }

    // イベントがシグナル状態になるまで待機します
    void wait()
    {
        _semaphore.acquire();
        _flag = false;
    }

    // イベントがシグナル状態になるか、stop_tokenが停止状態を示すまで待機します
    template<class Rep, class Period>
    bool try_wait(const std::stop_token& token, const std::chrono::duration<Rep, Period>& rel_time)
    {
        while(!token.stop_requested()) {
            if(_semaphore.try_acquire_for(rel_time)) {
                _flag = false;
                return true;
            }
        }
        return false;
    }

private:
    std::atomic<bool> _flag;
    std::binary_semaphore _semaphore;
};

}
