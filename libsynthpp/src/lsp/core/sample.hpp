#pragma once

#include <lsp/core/base.hpp>
#include <lsp/core/logging.hpp>

namespace lsp
{
/// サンプル型情報 
template<class T> requires std::signed_integral<T> || std::floating_point<T>
struct sample_traits {};

template<std::signed_integral T> struct sample_traits<T> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = T;
	static constexpr sample_type abs_max = std::numeric_limits<T>::max(); // C++20より、整数値は2の補数であることが保証された。 そのため絶対値の小さい正の最大値を基準に用いる。
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};
template<std::floating_point T> struct sample_traits<T> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = T;
	static constexpr sample_type abs_max = static_cast<T>(1.0);
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};

// ---

// ノーマライズ : 値域を信号の標準的な幅に狭める
template<class sample_type> requires std::signed_integral<sample_type> || std::floating_point<sample_type>
constexpr sample_type normalize(sample_type in) noexcept 
{
	// MEMO できるだけconstexprで解決し、実行時コストを純粋に変換処理のみとしたい。
	// MEMO 整数型でもノーマライズは必要 (2の補数を用いている処理系の場合、負の方向に1減らす必要が有るかもしれない)

	constexpr auto normalized_min = sample_traits<sample_type>::normalized_min;
	constexpr auto normalized_max = sample_traits<sample_type>::normalized_max;

	return std::clamp(in, normalized_min, normalized_max);
}

// 再量子化 : サンプルのフォーマットを変更
template<
	class Tout,
	class Tin
>requires 
	(std::signed_integral<Tout> || std::floating_point<Tout>) &&
	(std::signed_integral<Tin> || std::floating_point<Tin>)
constexpr Tout requantize(Tin in) noexcept 
{
	// MEMO できるだけconstexprで解決し、実行時コストを純粋に変換処理のみとしたい。

	if constexpr (std::is_same_v<Tin, Tout>) {
		// 入力と出力の型が同一であれば変換不要
		return in;
	} else if constexpr (std::is_floating_point_v<Tin> && std::is_floating_point_v<Tout>) {
		// 浮動小数点数同士は値域変換可能, クリッピング不要
		return static_cast<Tout>(in);
	} else if constexpr (std::is_integral_v<Tin> && std::is_integral_v<Tout>) {
		// 整数同士はシフト演算のみで値域変換可能, クリッピング不要
		// ただし、符号付整数のシフト演算は処理系依存の警告が出るため、乗算/除算で置き換える
		// 最終的にはコンパイル時に最適化されるためパフォーマンスには影響なし
		constexpr size_t in_bits = sizeof(Tin) * 8;
		constexpr size_t out_bits = sizeof(Tout) * 8;
		if constexpr (in_bits > out_bits) {
			// ナローイング変換(右シフト)
			return static_cast<Tout>(in / (1 << (in_bits - out_bits)));
		} else {
			// ワイドニング変換(左シフト)
			return static_cast<Tout>(in * (1 << (out_bits - in_bits)));
		}
	} else if constexpr (std::is_floating_point_v<Tin> && std::is_integral_v<Tout>) {
		// 浮動小数点数→整数 : ノーマライズしてから増幅
		return static_cast<Tout>(normalize(in) * sample_traits<Tout>::abs_max);
	} else if constexpr (std::is_integral_v<Tin> && std::is_floating_point_v<Tout>) {
		// 整数→浮動小数点数 : 最大振れ幅で割るだけ
		return static_cast<Tout>(in) / sample_traits<Tin>::abs_max;
	} else {
		// 変換未対応
		static_assert(false_v, "Unsupported type.");
	}
}

///
}