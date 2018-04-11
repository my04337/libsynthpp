#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Logging.hpp>

namespace LSP
{

// TODO いずれ信号型で固定長バッファからのアロケートを行いたい

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

// 信号用メモリプール
class SignalPool final
	: non_copy_move
{
public:
	// 信号用メモリ 保持クラス
	template<typename signal_type>
	class SignalHolder;

public:
	SignalPool() {}
	SignalPool(std::pmr::memory_resource* upstream) : mMem(upstream) {}
	SignalPool(const std::pmr::pool_options& opts) : mMem(opts) {}
	SignalPool(const std::pmr::pool_options& opts, std::pmr::memory_resource* upstream) : mMem(opts, upstream) {}


	// 信号用メモリを確保します(1チャネル用)
	template<typename signal_type, class = std::enable_if_t<is_sample_type_v<signal_type>>>
	SignalHolder<signal_type> allocate(size_t frames) {
		return allocate<signal_type>(1, frames);
	}

	// 信号用メモリを確保します(任意チャネル用)
	template<typename signal_type, class = std::enable_if_t<is_sample_type_v<signal_type>>>
	SignalHolder<signal_type> allocate(uint32_t channels, size_t frames) {
		auto data = allocate_memory<signal_type>(mMem, channels*frames);
		return SignalHolder<signal_type>(std::move(data), channels, frames);
	}

private:
	std::pmr::synchronized_pool_resource mMem; 
};

// ---

template<typename signal_type>
class SignalPool::SignalHolder final
	: non_copy
{
	friend class SignalPool;
public:
	SignalHolder(SignalHolder&& d)noexcept = default;
	SignalHolder& operator=(SignalHolder&& d)noexcept = default;

	// チャネル数を取得します
	uint32_t channels()const noexcept { return mChannels; }

	// フレーム数を取得します
	size_t frames()const noexcept { return mFrames; }

	// 各フレームの先頭ポインタを取得します
	signal_type* frame(size_t frame_index)const noexcept { return mData.get() + mChannels * frame_index; }

	// 全データへのポインタを取得します
	signal_type* data()const noexcept { return mData.get(); }

protected:
	SignalHolder(std::unique_ptr<signal_type[], _memory_resource_deleter<signal_type>>&& data, uint32_t channels, size_t frames) 
		: mData(std::move(data)), mChannels(channels), mFrames(frames) {}

private:
	std::unique_ptr<signal_type[], _memory_resource_deleter<signal_type>> mData;
	uint32_t mChannels;
	size_t mFrames;
};

}