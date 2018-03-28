#pragma once

#include <LSP/minimal.hpp>

#ifndef WIN32
#error unsupported platform
#endif
#ifndef WIN32
#error unsupported platform
#endif

#include <Windows.h>
#include <Mmreg.h>
#include <atlbase.h>
#include <atlcom.h>

struct IAudioClient3;

namespace LSP::Audio {

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
	template<typename signal_type, typename Tintermediate = double>
	void write(const Signal<signal_type>& ch1);
	template<typename signal_type, typename Tintermediate = double>
	void write(const Signal<signal_type>& ch1, const Signal<signal_type>& ch2);
	template<typename signal_type, typename Tintermediate = double>
	void write(const Signal<signal_type>* sigs[], size_t num);

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


template<typename signal_type, typename Tintermediate>
void WasapiOutput::write(const Signal<signal_type>& ch1)
{
	const Signal<signal_type>* sigs[] = {&ch1};
	write<signal_type, Tintermediate>(sigs, 1);
}
template<typename signal_type, typename Tintermediate>
void WasapiOutput::write(const Signal<signal_type>& ch1, const Signal<signal_type>& ch2)
{
	const Signal<signal_type>* sigs[] = {&ch1, &ch2};
	write<signal_type, Tintermediate>(sigs, 2);
}

template<typename signal_type, typename Tintermediate>
void WasapiOutput::write(const Signal<signal_type>* signals[], size_t signal_num)
{
	if(signal_num == 0) return;

	if (!valid()) {
		Log::e(LOGF("WasapiOutput : write - failed (invalid)"));
		return;
	}
	
	const auto signal_length = signals[0]->length();
	for (size_t ch=1; ch<signal_num; ++ch) {
		lsp_assert_desc(signals[ch]->size() == signal_length, "WasapiOutput : write - failed (signal length is mismatch)");
	}
	if(signal_length == 0) return;
	const auto channel_num = getDeviceChannels();

	std::lock_guard<decltype(mAudioBufferMutex)> lock(mAudioBufferMutex);

	auto safeGetSample = [&](size_t ch, size_t i)->signal_type {
		if (ch < signal_num) {
			return signals[ch]->data()[i];
		} else {
			return static_cast<signal_type>(0);
		}
	};

	switch (mSampleFormat) {
	case SampleFormat::Unknown:
		lsp_assert(false);
		break;
	case SampleFormat::Int8:
	case SampleFormat::Int16:
	case SampleFormat::Int24:
	case SampleFormat::Int32:
		for(size_t i=0; i<signal_length; ++i) {
			for (size_t ch=0; ch<channel_num; ++ch) {
				auto s = SampleFormatConverter<signal_type, int32_t, Tintermediate>::convert(safeGetSample(ch, i));
				mAudioBuffer.push_back(s);
			}
		}
		break;
	case SampleFormat::Float32:
		for(size_t i=0; i<signal_length; ++i) {
			for (size_t ch=0; ch<channel_num; ++ch) {
				auto s = SampleFormatConverter<signal_type, float, Tintermediate>::convert(safeGetSample(ch, i));
				mAudioBuffer.push_back(s);
			}
		}
		break;
	}
	SetEvent(mAudioEvent);
}

}
