#include <LSP/Audio/WasapiOutput.hpp>
#include <LSP/Debugging/Logging.hpp>
#include <LSP/Base/Math.hpp>

#include <Mmdeviceapi.h>
#include <Audioclient.h>



using namespace LSP;
using namespace LSP::Windows;

// 参考URL : https://charatsoft.sakura.ne.jp/develop/toaru2/index.php?did=7

WasapiOutput::WasapiOutput()
	: mAudioEvent(nullptr)
	, mAudioThreadHandle(nullptr)
	, mAbort(false)
{
	mAudioEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	lsp_assert(mAudioEvent != nullptr);
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
}

bool WasapiOutput::valid() const noexcept
{
	return mAudioClient != nullptr;
}

bool WasapiOutput::initialize(uint32_t sampleFreq, uint32_t bitsPerSample, uint32_t channels)
{
	if(valid()) {
		Log::e(LOGF("WasapiOutput : initialize - failed (already initialized)"));
		return false;
	}

	HRESULT hr;
	
	// マルチメディアデバイス列挙子
	CComPtr<IMMDeviceEnumerator> pDeviceEnumerator;
	if (!SUCCEEDED(hr = pDeviceEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL))) {
		Log::e(LOGF("WasapiOutput : initialize - failed (IMMDeviceEnumerator)"));
		return false;
	}
	
	// デフォルトのデバイスを選択
	CComPtr<IMMDevice> pDevice;
	if (!SUCCEEDED(hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice))) {
		Log::e(LOGF("WasapiOutput : initialize - failed (IMMDevice)"));
		return false;
	}

	// オーディオクライアントを取得
	CComPtr<IAudioClient> pAudioClient;
	if (!SUCCEEDED(hr = pDevice->Activate(_uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&pAudioClient)))) {
		Log::e(LOGF("WasapiOutput : initialize - failed (IMMDevice)"));
		return false;
	}

	// フォーマットの初期化
	bool autoConvert = false;
	WAVEFORMATEXTENSIBLE wf;
	ZeroMemory( &wf,sizeof(wf) );
	wf.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
	wf.Format.wFormatTag            = WAVE_FORMAT_EXTENSIBLE;
	wf.Format.nChannels             = channels;
	wf.Format.nSamplesPerSec        = sampleFreq;
	wf.Format.wBitsPerSample        = bitsPerSample;
	wf.Format.nBlockAlign           = wf.Format.nChannels * wf.Format.wBitsPerSample / 8;
	wf.Format.nAvgBytesPerSec       = wf.Format.nSamplesPerSec * wf.Format.nBlockAlign;
	wf.Samples.wValidBitsPerSample  = wf.Format.wBitsPerSample;
	wf.dwChannelMask                = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
	wf.SubFormat                    = KSDATAFORMAT_SUBTYPE_PCM;
	auto unitFrameSize = wf.Format.nBlockAlign;
	WAVEFORMATEXTENSIBLE* pClosestMatch = nullptr;
	auto fin_act_freeClosetMatch = finally([&pClosestMatch]{if(pClosestMatch) CoTaskMemFree(pClosestMatch); pClosestMatch = nullptr;});
	if (!SUCCEEDED(hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX*)&wf, (WAVEFORMATEX**)&pClosestMatch))) {
		Log::e(LOGF("WasapiOutput : initialize - failed (unsupported format)"));
		return false;
	}
	if (hr == S_FALSE && pClosestMatch) {
		// そのままズバリのフォーマットではない場合、フォーマット変換を挟む
		autoConvert = true;
	}

	// レイテンシ設定
	REFERENCE_TIME default_device_period = 0; // 単位 : 100ナノ秒
	REFERENCE_TIME minimum_device_period = 0;  // 単位 : 100ナノ秒
	if (!SUCCEEDED(hr = pAudioClient->GetDevicePeriod(&default_device_period, &minimum_device_period))) {
		Log::w(LOGF("WasapiOutput : Could not get default device period. Set default value."));
		default_device_period = minimum_device_period = 50*10; // 50ミリ秒
	}

	// 初期化
	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK 
			| (autoConvert ? AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM : 0),
		default_device_period,              // デフォルトデバイスピリオド値をセット
		default_device_period,              // デフォルトデバイスピリオド値をセット
		(WAVEFORMATEX*)&wf,
		NULL
	); 
	if (!SUCCEEDED(hr)) {
		Log::e(LOGF("WasapiOutput : initialize - failed (initialize)"));
		return false;
	}

	// イベントセット
	if (!(SUCCEEDED(hr = pAudioClient->SetEventHandle(mAudioEvent)))) {
		Log::e(LOGF("WasapiOutput : initialize - failed (SetEventHandle)"));
		return false;
	}

	// レンダラ取得
	CComPtr<IAudioRenderClient> pRenderClient;
	if (!SUCCEEDED(hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient))) {
		Log::e(LOGF("WasapiOutput : initialize - failed (IAudioRenderClient)"));
		return false;
	}

	// バッファの明示的ゼロクリア
	UINT32 availableFrameCount = 0;
	if (SUCCEEDED(hr = pAudioClient->GetBufferSize(&availableFrameCount))) {
		LPBYTE pData;
		if (SUCCEEDED(hr = pRenderClient->GetBuffer(availableFrameCount, &pData))) {
			ZeroMemory(pData, availableFrameCount * unitFrameSize);
			pRenderClient->ReleaseBuffer(availableFrameCount, 0);
		}
	}

	// 最終準備 : 再生用スレッド始動
	auto hAudioThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &playThreadMainProxy, this, CREATE_SUSPENDED, nullptr));
	if (!hAudioThread) {
		Log::e(LOGF("WasapiOutput : initialize - failed (_beginthreadex)"));
		return false;
	}
	mAudioThreadHandle = hAudioThread;
	SetThreadPriority(hAudioThread, THREAD_PRIORITY_HIGHEST);
	ResumeThread(hAudioThread);

	// OK
	Log::i(LOGF(
		"WasapiOutput : initialized";
		if(autoConvert) _ << " (resampling is enabled)";
			));
	mAudioClient = pAudioClient;
	mSampleFreq = sampleFreq;
	mBitsPerSample = bitsPerSample;
	mChannels = channels;
	mUnitFrameSize = unitFrameSize;

	return true;
}

bool WasapiOutput::start()
{
	if (!valid()) {
		Log::e(LOGF("WasapiOutput : start - failed (invalid)"));
		return false;
	}
	HRESULT hr;
	if (!(SUCCEEDED(hr = mAudioClient->Start()))) {
		Log::e(LOGF("WasapiOutput : start - failed (Start)"));
		return false;
	}
	SetEvent(mAudioEvent);
	return true;
}

bool WasapiOutput::stop()
{
	if (!valid()) {
		Log::e(LOGF("WasapiOutput : stop - failed (invalid)"));
		return false;
	}
	HRESULT hr;
	if (!(SUCCEEDED(hr = mAudioClient->Stop()))) {
		Log::e(LOGF("WasapiOutput : stop - failed (Stop)"));
		return false;
	}
	SetEvent(mAudioEvent);
	return true;
}

// 再生待ちサンプル数を取得します
size_t WasapiOutput::buffered_count()const noexcept
{
	std::lock_guard<decltype(mAudioBufferMutex)> lock(mAudioBufferMutex);
	return mAudioBuffer.size() / mChannels;
}

// ----------------------------------------------------------------------------

unsigned WasapiOutput::playThreadMainProxy(void* this_)
{
	reinterpret_cast<WasapiOutput*>(this_)->playThreadMain();
	return 0;
}

void WasapiOutput::playThreadMain()
{
	Log::d(LOGF("WasapiOutput : playThread begin"));
	auto fin_act_thread_end = finally([]{ Log::d(LOGF("WasapiOutput : playThread end"));} );

	HRESULT hr;
	CComPtr<IAudioRenderClient> pRenderClient;
	if (!SUCCEEDED(mAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient))) {
		Log::e(LOGF("WasapiOutput : playThread - failed (GetService:IAudioRenderClient)"));
		return;
	}

	// 各種定数群 (メンバ変数アクセスとするより高速にアクセス可能
	const uint32_t sampleFreq = mSampleFreq;
	const uint32_t bitsPerSample = mBitsPerSample;
	const uint32_t bytesPerSample = mBitsPerSample / 8;
	const uint32_t channels = mChannels;
	const uint32_t unitFrameSize = mUnitFrameSize;


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
			Log::w(LOGF("WasapiOutput : playThread - failed (GetBufferSize)"));
			continue;
		}
		UINT32 paddingFrameCount = 0;
		if (!SUCCEEDED(hr = mAudioClient->GetCurrentPadding(&paddingFrameCount))) {
			Log::w(LOGF("WasapiOutput : playThread - failed (GetCurrentPadding)"));
			continue;
		}
		lsp_assert(maxFrameCount >= paddingFrameCount);
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
			Log::w(LOGF("WasapiOutput : playThread - failed (GetBuffer)"));
			continue;
		}
		for (UINT32 i = 0; i < writingFrameCount; ++i) {
			auto pFrame = reinterpret_cast<char*>(pBuffer + i*unitFrameSize);
			for (size_t ch = 0; ch < channels; ++ch) {
				int32_t s = mAudioBuffer.front();
				mAudioBuffer.pop_front();
				s >>= 32 - bitsPerSample; // 32bitで記録しているので、必要サイズに併せて切り詰める
				// MEMO リトルエンディアン前提コード, 下位側から必要バイト分を転写
				memcpy(pFrame + bytesPerSample * ch, &s, bytesPerSample); 
			}
		}
		hr = pRenderClient->ReleaseBuffer(writingFrameCount, 0);
		Log::v(LOGF("WasapiOutput : playThread - wrote " << writingFrameCount << " samples"));

	}

}