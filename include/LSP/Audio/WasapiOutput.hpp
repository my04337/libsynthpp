#pragma once

#include <LSP/Base/Base.hpp>

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

protected:
	static unsigned __stdcall playThreadMainProxy(void*);
	void playThreadMain();

private:
	CComPtr<IAudioClient> mAudioClient;
	HANDLE mAudioEvent;
	HANDLE mAudioThreadHandle;
	std::atomic_bool mAbort;

	// --- valid時のみ有効 ---
	uint32_t mSampleFreq;
	uint32_t mBitsPerSample;
	uint32_t mChannels;
	uint32_t mUnitFrameSize;

};

}
}