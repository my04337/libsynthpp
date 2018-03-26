#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>
#include <LSP/Debugging/Logging.hpp>

#ifndef WIN32
#error unsupported platform
#endif

#include <Windows.h>
#include <atlbase.h>
#include <atlcom.h>

struct IAudioClient;

namespace LSP {
namespace Windows {

// WASAPI 出力
class WasapiOutput final
	: non_copy_move
{
public:
	WasapiOutput();
	~WasapiOutput();

	// 有効な状態か否かを取得します
	bool valid() const noexcept;

	// デバイスを初期化します
	bool initialize(uint32_t sampleFreq, uint32_t bitsPerSample, uint32_t channels);

	// デバイスからの出力を開始します
	bool start();

	// デバイスからの出力を停止します
	bool stop();

	// 再生待ちサンプル数を取得します
	size_t buffered_count()const noexcept;

	// 信号を書き込みます
	template<typename signal_type, typename Tintermediate = double>
	bool write(const Signal<signal_type>& ch1);
	template<typename signal_type, typename Tintermediate = double>
	bool write(const Signal<signal_type>& ch1, const Signal<signal_type>& ch2);
	template<typename signal_type, typename Tintermediate = double>
	bool write(const Signal<signal_type>* sigs[], size_t num);

protected:
	static unsigned __stdcall playThreadMainProxy(void*);
	void playThreadMain();


private:
	CComPtr<IAudioClient> mAudioClient;
	HANDLE mAudioEvent;
	HANDLE mAudioThreadHandle;
	std::atomic_bool mAbort;

	mutable std::mutex mAudioBufferMutex;
	std::deque<int32_t>  mAudioBuffer; // 簡単化のため、内部的にはint32で保持する

	// --- valid時のみ有効 ---
	uint32_t mSampleFreq;
	uint32_t mBitsPerSample;
	uint32_t mChannels;
	uint32_t mUnitFrameSize;

};

// ---

template<typename signal_type, typename Tintermediate>
bool WasapiOutput::write(const Signal<signal_type>& ch1)
{
	return write<signal_type, Tintermediate>(&ch1, 1);
}
template<typename signal_type, typename Tintermediate>
bool WasapiOutput::write(const Signal<signal_type>& ch1, const Signal<signal_type>& ch2)
{
	const Signal<signal_type>* sigs[] = {&ch1, &ch2};
	return write<signal_type, Tintermediate>(sigs, 2);
}

template<typename signal_type, typename Tintermediate>
bool WasapiOutput::write(const Signal<signal_type>* sigs[], size_t num)
{
	if(num == 0) return true;

	if (!valid()) {
		Log::e(LOGF(L"WasapiOutput : write - failed (invalid)"));
		return false;
	}
	if (num != mChannels) {
		Log::e(LOGF(L"WasapiOutput : write - failed (channel count is mismatch)"));
		return false;
	}
	const auto sz = sigs[0]->size();
	for (size_t ch=1; ch< num; ++ch) {
		if (sigs[ch]->size() != sz) {
			Log::e(LOGF(L"WasapiOutput : write - failed (signal length is mismatch)"));
			return false;
		}
	}

	std::lock_guard<decltype(mAudioBufferMutex)> lock(mAudioBufferMutex);

	for(size_t i=0; i<sz; ++i) {
		for (size_t ch=0; ch< num; ++ch) {
			auto s = SampleFormatConverter<signal_type, int32_t, Tintermediate>::convert(sigs[ch]->data()[i]);
			mAudioBuffer.push_back(s);
		}
	}

	return true;
}

}
}