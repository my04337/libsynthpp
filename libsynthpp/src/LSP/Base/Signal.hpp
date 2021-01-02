#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Sample.hpp>

namespace LSP
{

// 信号 view
template<sample_typeable sample_type>
class SignalView
{
public:
	constexpr SignalView(const sample_type* data, uint32_t channels, size_t frames)
		: mData(data), mChannels(channels), mFrames(frames) {}

	// チャネル数を取得します
	constexpr uint32_t channels()const noexcept { return mChannels; }

	// フレーム数を取得します
	constexpr size_t frames()const noexcept { return mFrames; }

	// 各フレームの先頭ポインタを取得します
	constexpr sample_type* frame(size_t frame_index)const noexcept { return mData + mChannels * frame_index; }

	// 全データへのポインタを取得します
	constexpr sample_type* data()const noexcept { return mData; }

private:
	const sample_type* mData;
	uint32_t mChannels;
	size_t mFrames;
};

// 信号型
template<sample_typeable sample_type>
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
		return allocate<sample_type>(mem, 1, frames);
	}
	static Signal allocate(std::pmr::memory_resource* mem, uint32_t channels, size_t frames) {
		auto data = allocate_memory<sample_type>(mem, channels*frames);
		return Signal(std::move(data), channels, frames);
	}

	Signal() : mData(), mChannels(1), mFrames(0) {}

	Signal(Signal&& d)noexcept = default;
	Signal& operator=(Signal&& d)noexcept = default;

	operator SignalView<sample_type> ()const noexcept { return signal_view<sample_type>(mData.get(), mChannels, mFrames); }

	// チャネル数を取得します
	uint32_t channels()const noexcept { return mChannels; }

	// フレーム数を取得します
	size_t frames()const noexcept { return mFrames; }

	// 各フレームの先頭ポインタを取得します
	sample_type* frame(size_t frame_index)const noexcept { return mData.get() + mChannels * frame_index; }

	// 全データへのポインタを取得します
	sample_type* data()const noexcept { return mData.get(); }

protected:
	Signal(std::unique_ptr<sample_type[], _memory_resource_deleter<sample_type>>&& data, uint32_t channels, size_t frames) 
		: mData(std::move(data)), mChannels(channels), mFrames(frames) {}

private:
	std::unique_ptr<sample_type[], _memory_resource_deleter<sample_type>> mData;
	uint32_t mChannels;
	size_t mFrames;
};

}