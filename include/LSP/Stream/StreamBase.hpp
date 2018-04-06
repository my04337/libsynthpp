#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::Stream
{

// --- 基本的な型 ---

// 再生位置
using position_t = int64_t;
constexpr position_t MAX_POSITION = std::numeric_limits<position_t>::max();



}