#pragma once

#include <luath_gui/base/base.hpp>

namespace luath_gui::widget
{

class Lissajous
{
public:
	Lissajous();
	~Lissajous();

	// 信号パラメータを設定します
	void setSignalParams(uint32_t sampleFreq, uint32_t channels, uint32_t bufferLength);

	// 表示波形を書き込みます
	template<typename sample_type>
	void write(const lsp::Signal<sample_type>& sig);


	// リサージュ曲線を描画を描画します
	void draw(ID2D1RenderTarget& renderer, float x, float y, float width, float height);
	
private:
	void _reset();

	mutable std::mutex mMutex;
	uint32_t mSampleFreq = 1;
	uint32_t mChannels = 1; // 受信チャネル
	uint32_t mBufferLength = 1;
	std::vector<std::deque<float>> mBuffers; // リングバッファ
};

template<typename sample_type>
void Lissajous::write(const lsp::Signal<sample_type>& sig)
{
	std::lock_guard lock(mMutex);

	const auto signal_channels = sig.channels();
	const auto signal_frames = sig.frames();
	const auto buffer_length = mBufferLength;

	lsp::require(signal_channels == mChannels, "WasapiOutput : write - failed (channel count is mismatch)");


	for (size_t ch=0; ch<signal_channels; ++ch) {
		auto& buff = mBuffers[ch];
		for(size_t i=0; i<signal_frames; ++i) {
			buff.push_back(requantize<float>(sig.frame(i)[ch]));
		}
		while(buff.size() > buffer_length) buff.pop_front();
	}
}

//
}