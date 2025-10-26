#include <lsp/audio/wasapi_output.hpp>

#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <ksmedia.h>



using namespace lsp;
using namespace lsp::audio;

// 参考URL : https://charatsoft.sakura.ne.jp/develop/toaru2/index.php?did=7

WasapiOutput::WasapiOutput()
	: mAudioEvent(nullptr)
	, mAudioThreadHandle(nullptr)
	, mAbort(false)
	, mWaveFormatEx(nullptr)
	, mAudioBufferFrameCount(0)
	, mSampleFormat(SampleFormat::Unknown)
{
	mAudioEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	check(mAudioEvent != nullptr);

	initialize();
}
WasapiOutput::~WasapiOutput()
{
	// 出力用スレッド : 停止準備
	mAbort = true;
	if(mAudioEvent) {
		SetEvent(mAudioEvent);
	}

	// 出力用スレッド : 停止
	if (mAudioThreadHandle) {
		WaitForSingleObject(mAudioThreadHandle, INFINITE);
		CloseHandle(mAudioThreadHandle);
		mAudioThreadHandle = nullptr;
	}
	
	// 同期用イベント : 削除
	if(mAudioEvent) {
		CloseHandle(mAudioEvent);
		mAudioEvent = nullptr;
	}

	// オーディオクライアント 解放
	mAudioClient = nullptr;
	
	// キャッシュしていたフォーマット情報 解放
	if (mWaveFormatEx) {
		CoTaskMemFree(mWaveFormatEx);
		mWaveFormatEx = nullptr;
	}
}

bool WasapiOutput::valid() const noexcept
{
	return mAudioClient != nullptr;
}

void WasapiOutput::initialize()
{
	if(valid()) {
		Log::e("WasapiOutput : initialize - failed (already initialized)");
		return;
	}
	HRESULT hr;

	// オーディオクライアントの取得
	auto pAudioClient = getDefaultAudioClient();
	if(!pAudioClient) return;

	// デフォルトのフォーマット取得
	WAVEFORMATEX* pWaveFormatEx = nullptr;
	if (!SUCCEEDED(hr = pAudioClient->GetMixFormat(&pWaveFormatEx))) {
		Log::e("WasapiOutput : initialize - failed (getting mix format)");
		return;
	}
	check(pWaveFormatEx != nullptr);
	auto fin_act_freeMexedFormat = finally([&pWaveFormatEx]{CoTaskMemFree(pWaveFormatEx); pWaveFormatEx = nullptr;});

	// フォーマットが扱える種類かを判定する
	auto sampleFormat = SampleFormat::Unknown;
	auto pcmBitPerSampleToSampleType = [](WORD wBitsPerSample)->SampleFormat {
		switch(wBitsPerSample) {
		case 8:		return SampleFormat::Int8;
		case 16:	return SampleFormat::Int16;
		case 24:	return SampleFormat::Int24;
		case 32:	return SampleFormat::Int32;
		default:	return SampleFormat::Unknown;
		}
	};
	auto floatBitPerSampleToSampleType = [](WORD wBitsPerSample)->SampleFormat {
		switch(wBitsPerSample) {
		case 32:	return SampleFormat::Float32;
		default:	return SampleFormat::Unknown;
		}
	};
	if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_PCM) {
		sampleFormat = pcmBitPerSampleToSampleType(pWaveFormatEx->wBitsPerSample);
	} else if(pWaveFormatEx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		auto pWaveFormatExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pWaveFormatEx);
		if (pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
			sampleFormat = pcmBitPerSampleToSampleType(pWaveFormatExtensible->Format.wBitsPerSample);
		} else if (pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
			sampleFormat = floatBitPerSampleToSampleType(pWaveFormatExtensible->Format.wBitsPerSample);
		}
	}
	if (sampleFormat == SampleFormat::Unknown) {
		Log::e("WasapiOutput : initialize - failed (unsupported sample format)");
		return;
	}


	// レイテンシ設定
	UINT32 defaultPeriodInFrames = 0;
	UINT32 fundamentalPeriodInFrames = 0;
	UINT32 minPeriodInFrames = 0;
	UINT32 maxPeriodInFrame = 0;
	if (!SUCCEEDED(hr = pAudioClient->GetSharedModeEnginePeriod(pWaveFormatEx, &defaultPeriodInFrames, &fundamentalPeriodInFrames, &minPeriodInFrames, &maxPeriodInFrame))) {
		Log::e("WasapiOutput : initialize - failed (Could not get default device period)");
		return;
	}

	// 初期化
	hr = pAudioClient->InitializeSharedAudioStream(
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		defaultPeriodInFrames,
		pWaveFormatEx,
		NULL
	); 
	if (!SUCCEEDED(hr)) {
		Log::e("WasapiOutput : initialize - failed (initialize)");
		return;
	}

	// イベントセット
	if (!(SUCCEEDED(hr = pAudioClient->SetEventHandle(mAudioEvent)))) {
		Log::e("WasapiOutput : initialize - failed (SetEventHandle)");
		return;
	}

	// レンダラ取得 (バッファクリア用)
	CComPtr<IAudioRenderClient> pRenderClient;
	if (!SUCCEEDED(hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient))) {
		Log::e("WasapiOutput : initialize - failed (IAudioRenderClient)");
		return;
	}

	// バッファの明示的ゼロクリア
	UINT32 bufferFrameCount = 0;
	if (!SUCCEEDED(hr = pAudioClient->GetBufferSize(&bufferFrameCount))) {
		Log::e("WasapiOutput : initialize - failed (GetBufferSize)");
		return;
	}
	LPBYTE pData;
	if (SUCCEEDED(hr = pRenderClient->GetBuffer(bufferFrameCount, &pData))) {
		ZeroMemory(pData, bufferFrameCount * pWaveFormatEx->nBlockAlign);
		pRenderClient->ReleaseBuffer(bufferFrameCount, AUDCLNT_BUFFERFLAGS_SILENT);
	}

	// 最終準備 : 再生用スレッド初期化
	auto hAudioThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &playThreadMainProxy, this, CREATE_SUSPENDED, nullptr));
	if (!hAudioThread) {
		Log::e("WasapiOutput : initialize - failed (_beginthreadex)");
		return;
	}
	mAudioThreadHandle = hAudioThread;
	SetThreadPriority(hAudioThread, THREAD_PRIORITY_HIGHEST);

	// 最終準備 : 各種パラメータ保存
	mAudioClient = pAudioClient;
	mWaveFormatEx = pWaveFormatEx;
	mAudioBufferFrameCount = bufferFrameCount;
	mSampleFormat = sampleFormat;
	fin_act_freeMexedFormat.reset();

	// OK
	Log::i("WasapiOutput : initialized (shared mode)");
	Log::i("  sample freq => {}[Hz]", pWaveFormatEx->nSamplesPerSec);
	Log::i("  channels    => {}", pWaveFormatEx->nChannels);
	Log::i("  format      => {}", getDeviceFormatString(sampleFormat));
	ResumeThread(hAudioThread); // 再生用スレッド始動
}

bool WasapiOutput::start()
{
	if (!valid()) {
		Log::e("WasapiOutput : start - failed (invalid)");
		return false;
	}
	HRESULT hr;
	if (!(SUCCEEDED(hr = mAudioClient->Start()))) {
		Log::e("WasapiOutput : start - failed (Start)");
		return false;
	}
	SetEvent(mAudioEvent);
	return true;
}

bool WasapiOutput::stop()
{
	if (!valid()) {
		Log::e("WasapiOutput : stop - failed (invalid)");
		return false;
	}
	HRESULT hr;
	if (!(SUCCEEDED(hr = mAudioClient->Stop()))) {
		Log::e("WasapiOutput : stop - failed (Stop)");
		return false;
	}
	SetEvent(mAudioEvent);
	return true;
}

uint32_t WasapiOutput::getDeviceSampleFreq()const noexcept
{
	check(valid());
	return mWaveFormatEx->nSamplesPerSec;
}
uint32_t WasapiOutput::getDeviceChannels()const noexcept
{
	check(valid());
	return mWaveFormatEx->nChannels;
}
WasapiOutput::SampleFormat WasapiOutput::getDeviceFormat()const noexcept
{
	check(valid());
	return mSampleFormat;
}
std::string_view WasapiOutput::getDeviceFormatString()const noexcept
{
	check(valid());
	return getDeviceFormatString(mSampleFormat);
}
std::string_view WasapiOutput::getDeviceFormatString(SampleFormat format)noexcept
{
	switch (format) {
	case SampleFormat::Unknown:	return "Unknown";
	case SampleFormat::Int8:	return "Int8";
	case SampleFormat::Int16:	return "Int16";
	case SampleFormat::Int24:	return "Int24";
	case SampleFormat::Int32:	return "Int32";
	case SampleFormat::Float32:	return "Float32";
	}
	std::unreachable();
}
size_t WasapiOutput::getDeviceBufferFrameCount()const noexcept
{
	check(valid());
	return mAudioBufferFrameCount;
}

size_t WasapiOutput::getBufferedFrameCount()const noexcept
{
	std::lock_guard<decltype(mAudioBufferMutex)> lock(mAudioBufferMutex);
	if(mWaveFormatEx) [[likely]] {
		return mAudioBuffer.size() / mWaveFormatEx->nChannels;
	} else {
		return 0;
	}
	
}

// ----------------------------------------------------------------------------

CComPtr<IAudioClient3> WasapiOutput::getDefaultAudioClient()
{
	HRESULT hr;

	// マルチメディアデバイス列挙子
	CComPtr<IMMDeviceEnumerator> pDeviceEnumerator;
	if (!SUCCEEDED(hr = pDeviceEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL))) {
		Log::e("WasapiOutput : getDefaultAudioClient - failed (IMMDeviceEnumerator)");
		return {};
	}

	// デフォルトのデバイスを選択
	CComPtr<IMMDevice> pDevice;
	if (!SUCCEEDED(hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice))) {
		Log::e("WasapiOutput : getDefaultAudioClient - failed (IMMDevice)");
		return {};
	}

	// オーディオクライアントを取得
	CComPtr<IAudioClient3> pAudioClient;
	if (!SUCCEEDED(hr = pDevice->Activate(_uuidof(IAudioClient3), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&pAudioClient)))) {
		Log::e("WasapiOutput : getDefaultAudioClient - failed (IAudioClient3)");
		return {};
	}

	return pAudioClient;
}
// ----------------------------------------------------------------------------

unsigned WasapiOutput::playThreadMainProxy(void* this_)
{
	reinterpret_cast<WasapiOutput*>(this_)->playThreadMain();
	return 0;
}

void WasapiOutput::playThreadMain()
{
	Log::d("WasapiOutput : playThread begin");
	auto fin_act_thread_end = finally([]{ Log::d("WasapiOutput : playThread end");} );

	HRESULT hr;
	CComPtr<IAudioRenderClient> pRenderClient;
	if (!SUCCEEDED(mAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient))) {
		Log::e("WasapiOutput : playThread - failed (GetService:IAudioRenderClient)");
		return;
	}

	// 各種定数群 (メンバ変数アクセスとするより高速にアクセス可能)
	[[maybe_unused]] const uint32_t sampleFreq = mWaveFormatEx->nSamplesPerSec;
	const uint32_t bitsPerSample = mWaveFormatEx->wBitsPerSample;
	const uint32_t bytesPerSample = bitsPerSample / 8;
	const uint32_t channels = mWaveFormatEx->nChannels;
	const uint32_t unitFrameSize = mWaveFormatEx->nBlockAlign;
	const SampleFormat sampleType = mSampleFormat;


	while (true) {
		// 状態変化を待機
		// MEMO 試験用に一旦無効化
		auto waitResult = WaitForSingleObject(mAudioEvent, 1000);
		if(waitResult == WAIT_TIMEOUT) continue;
		if(mAbort) break;

		// 出力対象の決定
		// - 今回出力可能なバッファサイズを取得
		UINT32 maxFrameCount = 0;
		if (!SUCCEEDED(hr = mAudioClient->GetBufferSize(&maxFrameCount))) {
			Log::w("WasapiOutput : playThread - failed (GetBufferSize)");
			continue;
		}
		UINT32 paddingFrameCount = 0;
		if (!SUCCEEDED(hr = mAudioClient->GetCurrentPadding(&paddingFrameCount))) {
			Log::w("WasapiOutput : playThread - failed (GetCurrentPadding)");
			continue;
		}
		check(maxFrameCount >= paddingFrameCount);
		UINT32 availableFrameCount = maxFrameCount - paddingFrameCount;
		if(availableFrameCount == 0) {
			// no data
			continue;
		}

		// - 出力バッファのバッファサイズを取得
		std::lock_guard<decltype(mAudioBufferMutex)> lock(mAudioBufferMutex);
		UINT32 bufferedFrameCount = static_cast<UINT32>(mAudioBuffer.size() / channels);


		// - 出力するバッファサイズを決定
		UINT32 writingFrameCount = std::min(availableFrameCount, bufferedFrameCount);
		if(writingFrameCount == 0) {
			// no data
			continue;
		}

		// 出力バッファへ書き出し
		LPBYTE pBuffer = nullptr;
		if (!SUCCEEDED(hr = pRenderClient->GetBuffer(writingFrameCount, &pBuffer))) {
			Log::w("WasapiOutput : playThread - failed (GetBuffer)");
			continue;
		}
		switch(sampleType) {
		case SampleFormat::Unknown:
			std::unreachable();
		case SampleFormat::Int8:
		case SampleFormat::Int16:
		case SampleFormat::Int24:
		case SampleFormat::Int32:
			for (UINT32 i = 0; i < writingFrameCount; ++i) {
				auto pFrame = reinterpret_cast<char*>(pBuffer + i*unitFrameSize);
				for (size_t ch = 0; ch < channels; ++ch) {
					auto s = std::get<int32_t>(mAudioBuffer.front());
					mAudioBuffer.pop_front();
					s >>= 32 - bitsPerSample; // 32bitで記録しているので、必要サイズに併せて切り詰める
					// MEMO リトルエンディアン前提コード, 下位側から必要バイト分を転写
					memcpy(pFrame + bytesPerSample * ch, &s, bytesPerSample); 
				}
			}
			break;
		case SampleFormat::Float32:
			for (UINT32 i = 0; i < writingFrameCount; ++i) {
				auto pFrame = reinterpret_cast<char*>(pBuffer + i*unitFrameSize);
				for (size_t ch = 0; ch < channels; ++ch) {
					auto s = std::get<float>(mAudioBuffer.front());
					mAudioBuffer.pop_front();
					memcpy(pFrame + bytesPerSample * ch, &s, bytesPerSample); 
				}
			}
			break;
		}
		hr = pRenderClient->ReleaseBuffer(writingFrameCount, 0);
		Log::v("WasapiOutput : playThread - wrote {}  samples", writingFrameCount);
	}

}