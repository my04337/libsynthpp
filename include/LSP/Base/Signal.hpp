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
template<class memory_resource_type = std::pmr::synchronized_pool_resource>
class SignalPool final
	: non_copy_move
{
public:
	template<typename signal_type>
	class Holder final
		: non_copy
	{
		friend class SignalPool<memory_resource_type>;
	public:
		~Holder() {
			dispose();
		}
		Holder(Holder&& d)noexcept
			: mMem(d.mMem)
			, mData(d.mData)
			, mAlign(d.mAlign)
			, mFrames(d.mFrames)
			, mChannels(d.mChannels)
		{
			d.mData = nullptr;
		}
		Holder& operator=(Holder&& d)noexcept
		{
			dispose();
			mMem = d.mMem;
			mData = d.mData;
			mAlign = d.mAlign;
			mFrames = d.mFrames;
			mChannels = d.mChannels;

			d.mData = nullptr;

			return *this;
		}

		void dispose() {
			if (mData) {
				mMem->deallocate(mData, mFrames*mChannels*sizeof(signal_type), mAlign);
				mData = nullptr;
			}
		}

		// チャネル数を取得します
		uint32_t channels()const noexcept { return mChannels; }

		// フレーム数を取得します
		size_t frames()const noexcept { return mFrames; }

		// 各フレームの先頭ポインタを取得します
		signal_type* frame(size_t frame_index)const noexcept { return mData + mChannels * frame_index; }

		// 全データへのポインタを取得します
		signal_type* data()const noexcept { return mData; }
		
	protected:
		Holder(memory_resource_type* mem, signal_type* data, uint32_t channels, size_t frames, size_t align) 
			: mMem(mem), mData(data), mAlign(align), mFrames(frames), mChannels(channels) {}

	private:
		memory_resource_type* mMem;
		signal_type* mData;
		const size_t mAlign;
		const size_t mFrames;
		const uint32_t mChannels;
	};
	template<typename signal_type>
	friend class Holder;

public:
	template<class... Args>
	SignalPool(Args... args) : mMem(std::forward<Args>(args)...) {}

	template<typename signal_type, class = std::enable_if_t<is_sample_type_v<signal_type>>>
	Holder<signal_type> allocate(size_t frames) {
		return allocate<signal_type>(1, frames);
	}

	template<typename signal_type, class = std::enable_if_t<is_sample_type_v<signal_type>>>
	Holder<signal_type> allocate(uint32_t channels, size_t frames) {
		auto data = reinterpret_cast<signal_type*>(mMem.allocate(channels * frames * sizeof(signal_type), alignof(signal_type)));
		return Holder<signal_type>(&mMem, data, channels, frames, alignof(signal_type));
	}

private:
	memory_resource_type mMem; 
};

}