#pragma once

#include <lsp/core/core.hpp>

#ifndef WIN32
#error unsupported platform
#endif

#include <Windows.h>
#include <Mmreg.h>
#include <atlbase.h>
#include <atlcom.h>

struct IAudioClient3;

namespace lsp::audio {

// WASAPI 出力
class WasapiOutput final
	: non_copy_move
{
public:
	enum class SampleFormat 
	{
		Unknown,
		Int8,
		Int16,
		Int24,
		Int32,
		Float32,
	};

public:
	WasapiOutput();
	~WasapiOutput();

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
	static unsigned __stdcall playThreadMainProxy(void*);
	void playThreadMain();

	static CComPtr<IAudioClient3> getDefaultAudioClient();
	void initialize();

private:
	CComPtr<IAudioClient3> mAudioClient;
	HANDLE mAudioEvent;
	HANDLE mAudioThreadHandle;
	std::atomic_bool mAbort;

	mutable std::mutex mAudioBufferMutex;
	std::deque<std::variant<int32_t, float>>  mAudioBuffer; // 簡単化のため、内部的には最大サイズで保持する

	// --- valid時のみ有効 ---
	WAVEFORMATEX* mWaveFormatEx; // TODO CoTaskMemFreeでの解放
	uint32_t mAudioBufferFrameCount;
	SampleFormat mSampleFormat;

};

// ---

template<typename sample_type>
void WasapiOutput::write(const Signal<sample_type>& sig)
{
	const auto signal_channels = sig.channels();
	const auto signal_frames = sig.frames();

	if(signal_channels == 0) return;
	if(signal_frames == 0) return;

	if (!valid()) {
		Log::e("WasapiOutput : write - failed (invalid)");
		return;
	}
	
	const auto device_channels = getDeviceChannels();

	std::lock_guard<decltype(mAudioBufferMutex)> lock(mAudioBufferMutex);

	auto safeGetSample = [&](size_t ch, size_t i)->sample_type {
		auto frame = sig.frame(i);
		if (ch < signal_channels) {
			return frame[ch];
		} else {
			return static_cast<sample_type>(0);
		}
	};

	switch (mSampleFormat) {
	case SampleFormat::Unknown:
		std::unreachable();
	case SampleFormat::Int8:
	case SampleFormat::Int16:
	case SampleFormat::Int24:
	case SampleFormat::Int32:
		for(size_t i=0; i<signal_frames; ++i) {
			for (size_t ch=0; ch<device_channels; ++ch) {
				auto s = requantize<int32_t>(safeGetSample(ch, i));
				mAudioBuffer.push_back(s);
			}
		}
		break;
	case SampleFormat::Float32:
		for(size_t i=0; i<signal_frames; ++i) {
			for (size_t ch=0; ch<device_channels; ++ch) {
				auto s = requantize<float>(safeGetSample(ch, i));
				mAudioBuffer.push_back(s);
			}
		}
		break;
	}
	SetEvent(mAudioEvent);
}

}
