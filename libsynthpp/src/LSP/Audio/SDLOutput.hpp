#pragma once

#include <LSP/minimal.hpp>
#include <LSP/Threading/EventSignal.hpp>

struct SDL_AudioSpec;

namespace LSP::Audio {

// SDL Audio 出力
class SDLOutput final
	: non_copy_move
{
public:
	enum class SampleFormat 
	{
		Unknown,
		Int8,
		Int16,
		Int32,
		Float32,
	};

public:
	SDLOutput(uint32_t sampleFreq, uint32_t channels, SampleFormat format);
	~SDLOutput();

	// 有効な状態か否かを取得します
	[[nodiscard]]
	bool valid() const noexcept;

	// デバイスからの出力を開始します
	[[nodiscard]]
	bool start();

	// デバイスからの出力を停止します
	[[nodiscard]]
	bool stop();

	// デバイスのサンプリング周波数を取得します
	uint32_t getDeviceSampleFreq()const noexcept;

	// デバイスのチャネル数を取得します
	uint32_t getDeviceChannels()const noexcept;

	// デバイスのサンプルフォーマットを取得します
	SampleFormat getDeviceFormat()const noexcept;
	std::string_view getDeviceFormatString()const noexcept;
	static std::string_view getDeviceFormatString(SampleFormat format)noexcept;

	// デバイスのバッファサイズを取得します
	size_t getDeviceBufferFrameCount()const noexcept;

	// 再生待ちサンプル数を取得します
	size_t getBufferedFrameCount()const noexcept;

	// 信号を書き込みます
	template<typename sample_type>
	void write(const Signal<sample_type>& sig);

protected:
	void initialize(uint32_t sampleFreq, uint32_t channels, SampleFormat format);
	void _write(void* data, size_t len);

private:
	// --- valid時のみ有効 ---
	std::pmr::synchronized_pool_resource mMem; // 近いサイズのメモリを高頻度に要求するため使用
	uint32_t mAudioDeviceID;
	std::unique_ptr<SDL_AudioSpec> mAudioSpec;

	uint32_t mAudioBufferFrameCount;
	SampleFormat mSampleFormat;
	uint32_t mBytesPerSample;

};

// ---

template<typename sample_type>
void SDLOutput::write(const Signal<sample_type>& sig)
{
	const auto signal_channels = sig.channels();
	const auto signal_frames = sig.frames();

	if(signal_channels == 0) return;
	if(signal_frames == 0) return;

	if (!valid()) {
		Log::e(LOGF("WasapiOutput : write - failed (invalid)"));
		return;
	}

	const auto device_channels = getDeviceChannels();

	auto convAndWrite = [&](auto type_hint) {
		using to_type = decltype(type_hint);

		size_t out_buff_size = signal_frames * device_channels;
		auto out_buff = allocate_memory<to_type>(&mMem, out_buff_size);
		size_t out_buff_pos = 0;

		for(size_t i=0; i<signal_frames; ++i) {
			auto frame = sig.frame(i);
			for (size_t ch=0; ch<device_channels; ++ch) {
				if (ch < signal_channels) {
					out_buff[out_buff_pos] = requantize<to_type>(frame[ch]);
				} else {
					out_buff[out_buff_pos] = 0;
				}
				++out_buff_pos;				
			}
		}
		_write(out_buff.get(), out_buff_size*sizeof(to_type));
	};

	switch (mSampleFormat) {
	case SampleFormat::Unknown:
		lsp_assert(false);
		break;
	case SampleFormat::Int8:
		convAndWrite(int8_t(0));
		break;
	case SampleFormat::Int16:
		convAndWrite(int16_t(0));
		break;
	case SampleFormat::Int32: 
		convAndWrite(int32_t(0));
		break;
	case SampleFormat::Float32: 
		convAndWrite(float(0));
		break;
	}
}

}
