#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Sample.hpp>

namespace LSP
{

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