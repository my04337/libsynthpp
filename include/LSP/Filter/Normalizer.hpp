#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>

namespace LSP::Filter
{

// ノーマライザ : 値域を信号の標準的な幅に狭める
struct Normalizer final
{
	template<
		typename sample_type,
		class = std::enable_if_t<is_sample_type_v<sample_type>>
	>
	constexpr sample_type operator()(sample_type in) const noexcept 
	{
		// MEMO できるだけconstexprで解決し、実行時コストを純粋に変換処理のみとしたい。
		// MEMO 整数型でもノーマライズは必要 (2の補数を用いている処理系の場合、負の方向に1減らす必要が有るかもしれない)

		constexpr auto normalized_min = sample_traits<sample_type>::normalized_min;
		constexpr auto normalized_max = sample_traits<sample_type>::normalized_max;

		return std::clamp(in, normalized_min, normalized_max);
	}
};

}