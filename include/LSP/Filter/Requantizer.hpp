#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>
#include <LSP/Filter/Normalizer.hpp>

namespace LSP::Filter
{

// 再量子化 : サンプルのフォーマットを変更
template<
	typename Tin,
	typename Tout,
	class = std::enable_if_t<is_sample_type_v<Tin> && is_sample_type_v<Tout>>
>
struct Requantizer
{
	constexpr Tout operator()(Tin in) const noexcept 
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
			constexpr size_t in_bits = sizeof(Tin) * 8;
			constexpr size_t out_bits = sizeof(Tout) * 8;
			if constexpr (in_bits > out_bits) {
				// ナローイング変換(右シフト)
				return static_cast<Tout>(in >> (in_bits - out_bits));
			} else {
				// ワイドニング変換(左シフト)
				return static_cast<Tout>(in) << (out_bits - in_bits);
			}
		} else if constexpr (std::is_floating_point_v<Tin> && std::is_integral_v<Tout>) {
			// 浮動小数点数→整数 : ノーマライズしてから増幅
			return static_cast<Tout>(Filter::Normalizer<Tin>()(in) * sample_traits<Tout>::abs_max);
		} else if constexpr (std::is_integral_v<Tin> && std::is_floating_point_v<Tout>) {
			// 整数→浮動小数点数 : 最大振れ幅で割るだけ
			return static_cast<Tout>(in) / sample_traits<Tin>::abs_max;
		} else {
			// 変換未対応
			static_assert(false, "Unsupported type.");
		}
	}
};


}