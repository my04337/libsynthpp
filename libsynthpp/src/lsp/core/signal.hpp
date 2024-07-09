// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/base.hpp>
#include <lsp/core/sample.hpp>

namespace lsp
{

// 信号型 : ※ juce::AudioBufferのラッパ
template<class sample_type> requires std::signed_integral<sample_type> || std::floating_point<sample_type>
class Signal final
	: non_copy
{
public:
	// 信号用メモリを確保します(1チャネル用)
	static Signal allocate(size_t frames) {
		return allocate(1, frames);
	}
	static Signal allocate(uint32_t channels, size_t frames) {
		return Signal(channels, frames);
	}
	static Signal fromAudioBuffer(juce::AudioBuffer<sample_type>&& buffer) {
		return Signal(std::move(buffer));
	}

	Signal() : mBuffer() {}

	Signal(Signal&& d)noexcept = default;
	Signal& operator=(Signal&& d)noexcept = default;

	// 信号をJUCEのAudioBuffer形式でを取得します
	juce::AudioBuffer<sample_type>& buffer()noexcept { return mBuffer; }
	const juce::AudioBuffer<sample_type>& buffer()const noexcept { return mBuffer; }

	// チャネル数を取得します
	uint32_t channels()const noexcept { return static_cast<uint32_t>(mBuffer.getNumChannels()); }

	// サンプル数を取得します
	size_t samples()const noexcept { return static_cast<size_t>(mBuffer.getNumSamples()); }

	// 各サンプルの先頭ポインタを取得します(書き込み用)
	sample_type* mutableData(uint32_t channel)noexcept { return mBuffer.getWritePointer(static_cast<int>(channel)); }
	sample_type& mutableData(uint32_t channel, size_t index)noexcept { return *mBuffer.getWritePointer(static_cast<int>(channel, static_cast<int>(index))); }
	
	// 各サンプルの先頭ポインタを取得します(読み取り用)
	const sample_type* data(uint32_t channel)const noexcept { return mBuffer.getReadPointer(static_cast<int>(channel)); }
	sample_type data(uint32_t channel, size_t index)const noexcept { return mBuffer.getSample(static_cast<int>(channel), static_cast<int>(index)); }

	// 信号レベルを算出します
	const sample_type getRMSLevel(uint32_t channel)const noexcept { return mBuffer.getRMSLevel(static_cast<int>(channel), 0, mBuffer.getNumSamples()); }

protected:
	Signal(uint32_t channels, size_t frames)
		: mBuffer(static_cast<int>(channels), static_cast<int>(frames)) {
	}
	Signal(juce::AudioBuffer<sample_type>&& buffer)
		: mBuffer(std::move(buffer)) {
	}

private:
	juce::AudioBuffer<sample_type> mBuffer;
};

}