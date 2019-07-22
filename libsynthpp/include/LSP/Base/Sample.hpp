#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Logging.hpp>

namespace LSP
{

/// サンプル型情報 
template<typename sample_type> struct sample_traits {
	// MEMO abs(min) == abs(max)が成り立つようにすること
};
template<> struct sample_traits<int8_t> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = int8_t;
	static constexpr sample_type abs_max = 0x7Fi8;
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};
template<> struct sample_traits<int16_t> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = int16_t;
	static constexpr sample_type abs_max = 0x7FFFi16; 
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};
template<> struct sample_traits<int32_t> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = int32_t;
	static constexpr sample_type abs_max = 0x7FFFFFFFi32;
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};
template<> struct sample_traits<float> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = float;
	static constexpr sample_type abs_max = 1.0f;
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};
template<> struct sample_traits<double> {
	using _sample_type_tag = void; // for SFINAE
	using sample_type = double;
	static constexpr sample_type abs_max = 1.0;
	static constexpr sample_type normalized_max = +abs_max;
	static constexpr sample_type normalized_min = -abs_max;
};

// サンプル型であるか否か
template <class T, class = void>
struct is_sample_type : std::false_type {};
template <class T>
struct is_sample_type<T, std::void_t<typename sample_traits<T>::_sample_type_tag>> : std::true_type {};
template<class T>
constexpr bool is_sample_type_v = is_sample_type<T>::value;

template <class T, class = void>
struct is_integral_sample_type : std::false_type {};
template <class T>
struct is_integral_sample_type<T, std::enable_if_t<is_sample_type_v<T> && std::is_integral_v<T>>> : std::true_type {};
template<class T>
constexpr bool is_integral_sample_type_v = is_integral_sample_type<T>::value;

template <class T, class = void>
struct is_floating_point_sample_type : std::false_type {};
template <class T>
struct is_floating_point_sample_type<T, std::enable_if_t<is_sample_type_v<T> && std::is_floating_point_v<T>>> : std::true_type {};
template<class T>
constexpr bool is_floating_point_sample_type_v = is_floating_point_sample_type<T>::value;

// ---

// ノーマライズ : 値域を信号の標準的な幅に狭める
template<
	typename sample_type,
	class = std::enable_if_t<is_sample_type_v<sample_type>>
>
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
	typename Tout,
	typename Tin,
	class = std::enable_if_t<is_sample_type_v<Tin> && is_sample_type_v<Tout>>
>
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
		return static_cast<Tout>(normalize(in) * sample_traits<Tout>::abs_max);
	} else if constexpr (std::is_integral_v<Tin> && std::is_floating_point_v<Tout>) {
		// 整数→浮動小数点数 : 最大振れ幅で割るだけ
		return static_cast<Tout>(in) / sample_traits<Tin>::abs_max;
	} else {
		// 変換未対応
		static_assert(false, "Unsupported type.");
	}
}

///
}