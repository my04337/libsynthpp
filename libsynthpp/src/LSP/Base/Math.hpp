#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP
{

// 数学関数&定数群

//円周率
template <std::floating_point T>
static constexpr T PI    = static_cast<T>(3.14159265358979323846264338327950288419716939937510L);

//√2
template <std::floating_point T>
static constexpr T Sqrt2 = static_cast<T>(1.41421356237309504880168872420969807856967187537694L);

}