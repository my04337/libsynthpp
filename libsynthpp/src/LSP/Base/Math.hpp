#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::Math
{

// 数学関数&定数群

//円周率
template <std::floating_point T>
inline static constexpr T PI = std::numbers::pi_v<T>;


// 被除数が0以上の場合、常に戻り値が0異常になるfmod(波形テーブル音源で位相が遡るような変化をする場合に有用)
// https://ja.wikipedia.org/wiki/%E5%89%B0%E4%BD%99%E6%BC%94%E7%AE%97
template <std::floating_point T>
static constexpr T floored_division(T a, T b)
{
	return a - std::floor(a / b) * b;
}
}