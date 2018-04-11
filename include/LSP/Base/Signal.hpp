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

// 信号型
template<typename signal_type>
class Signal final
	: non_copy
{
public:
	// 信号用メモリを確保します(1チャネル用)
	static Signal allocate(size_t frames) {
		return allocate(std::pmr::get_default_resource(), 1, frames);
	}
	static Signal allocate(uint32_t channels, size_t frames) {
		return allocate(std::pmr::get_default_resource(), channels, frames);
	}
	static Signal allocate(std::pmr::memory_resource* mem, size_t frames) {
		return allocate<signal_type>(mem, 1, frames);
	}
	static Signal allocate(std::pmr::memory_resource* mem, uint32_t channels, size_t frames) {
		auto data = allocate_memory<signal_type>(mem, channels*frames);
		return Signal(std::move(data), channels, frames);
	}

	Signal() : mData(), mChannels(1), mFrames(0) {}

	Signal(Signal&& d)noexcept = default;
	Signal& operator=(Signal&& d)noexcept = default;

	// チャネル数を取得します
	uint32_t channels()const noexcept { return mChannels; }

	// フレーム数を取得します
	size_t frames()const noexcept { return mFrames; }

	// 各フレームの先頭ポインタを取得します
	signal_type* frame(size_t frame_index)const noexcept { return mData.get() + mChannels * frame_index; }

	// 全データへのポインタを取得します
	signal_type* data()const noexcept { return mData.get(); }

protected:
	Signal(std::unique_ptr<signal_type[], _memory_resource_deleter<signal_type>>&& data, uint32_t channels, size_t frames) 
		: mData(std::move(data)), mChannels(channels), mFrames(frames) {}

private:
	std::unique_ptr<signal_type[], _memory_resource_deleter<signal_type>> mData;
	uint32_t mChannels;
	size_t mFrames;
};

}