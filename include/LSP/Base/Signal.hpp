#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Logging.hpp>

namespace LSP
{

// TODO いずれ信号型で固定長バッファからのアロケートを行いたい

/// サンプル型情報 
template<typename sample_type> struct sample_traits {
	// MEMO abs(min) == abs(max)が成り立つようにする
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


// サンプルフォーマット変換
template<typename Tin, typename Tout, typename Tintermediate=double>
struct SampleFormatConverter
{
	constexpr Tout operator()(Tin in) noexcept {
		return convert(in);
	}

	static constexpr Tout convert(Tin in_) noexcept {
		// MEMO できるだけconstexprで解決し、実行時コストを純粋に変換処理のみとしたい。

		if constexpr (std::is_same_v<Tin, Tout>) {
			// 入力と出力の型が同一であれば変換不要
			return in_;
		} else if constexpr (std::is_floating_point_v<Tin> && std::is_floating_point_v<Tout>) {
			// 浮動小数点同士は値域変換可能, クリッピング不要
			return static_cast<Tout>(in_);
		} else if constexpr (std::is_integral_v<Tin> && std::is_integral_v<Tout>) {
			// 整数同士はシフト演算のみで値域変換可能, クリッピング不要
			constexpr size_t in_bits = sizeof(Tin) * 8;
			constexpr size_t out_bits = sizeof(Tout) * 8;
			if constexpr (in_bits > out_bits) {
				// ナローイング変換(右シフト)
				return static_cast<Tout>(in_ >> (in_bits - out_bits));
			} else {
				// ワイドニング変換(左シフト)
				return static_cast<Tout>(in_) << (out_bits - in_bits);
			}
		} else {
			// それ以外の型同士では変換が必要
			// - 値域変換係数算出
			constexpr auto in_abs_max = static_cast<Tintermediate>(sample_traits<Tin>::abs_max);
			constexpr auto out_abs_max = static_cast<Tintermediate>(sample_traits<Tout>::abs_max);
			constexpr auto amp_rate = out_abs_max / in_abs_max;
		
			const auto in = static_cast<Tintermediate>(in_);
			const auto raw_out = in * amp_rate;

			// - クリッピング
			if constexpr (std::is_floating_point_v<Tout>) {
				return static_cast<Tout>(raw_out);
			} else {
				constexpr auto out_min = static_cast<Tintermediate>(sample_traits<Tout>::normalized_min);
				constexpr auto out_max = static_cast<Tintermediate>(sample_traits<Tout>::normalized_max);
				return static_cast<Tout>(std::max(out_min, std::min(raw_out, out_max)));
			}
		}
	}
	static void convert(Tout* out_, const Tin* in_, size_t sz) noexcept {
		for (size_t i = 0; i < sz; ++i) {
			out_[i] = convert(in[i]);
		}
	}
};
// サンプルノーマライズ
template<typename T>
struct SampleNormalizer
{
	constexpr T operator()(T in) noexcept {
		return normalize(in);
	}

	static constexpr T normalize(T in) noexcept {
		// MEMO できるだけconstexprで解決し、実行時コストを純粋に変換処理のみとしたい。

		constexpr auto normalized_min = sample_traits<T>::normalized_min;
		constexpr auto normalized_max = sample_traits<T>::normalized_max;

		return std::max(normalized_min, std::min(in, normalized_max));
	}
	static void normalize(T* arr, size_t sz) noexcept {
		for (size_t i = 0; i < sz; ++i) {
			arr[i] = normalize(arr[i]);
		}
	}
};

// ---

/// 信号型
template<
	typename sample_type_,
	class = std::enable_if_t<is_sample_type_v<sample_type_>>
>
class Signal final
	: non_copy
{
public:
	using sample_type = sample_type_;

public:
	Signal(size_t size)
		: Signal(1, size)
	{
	}
	Signal(size_t channels, size_t size)
		: mSize(size)
	{
		for (size_t ch = 0; ch < channels; ++ch) {
			mData.emplace_back(std::make_unique<sample_type[]>(size));
		}
	}

	~Signal() {}

	Signal(Signal&&)noexcept = default;
	Signal& operator=(Signal&&)noexcept = default;
	
	// チャネル数を取得します
	size_t channels()const noexcept { return mData.size(); }

	// データへのポインタを取得します
	sample_type* data()const noexcept { return mData[0]; }
	sample_type* data(size_t ch)const noexcept { return mData[ch].get(); }

	// データ数を取得します。
	size_t size()const noexcept { return mSize; }
	size_t length()const noexcept { return mSize; }	
	
private:
	const size_t mSize;
	std::vector<std::unique_ptr<sample_type[]>> mData;
};


}