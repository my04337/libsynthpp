#include <luath/window/main_window.hpp>
#include <luath/app/application.hpp>

#include <shellapi.h>

#include <array>
#include <map>
#include <map>
#include <numeric>
#include <fstream>

using namespace lsp;
using namespace luath;
using namespace luath::window;

using lsp::synth::LuathVoice;
using lsp::synth::VoiceId;

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 680;

static constexpr D2D1_COLOR_F getMidiChannelColor(int ch)
{
	switch(ch) {
	case  1: return D2D1_COLOR_F{ 1.f, 0.f, 0.f, 1.f }; // 赤
	case  2: return D2D1_COLOR_F{ 1.f, 0.5f, 0.f, 1.f }; // 朱色
	case  3: return D2D1_COLOR_F{ 1.f, 0.75f, 0.f, 1.f }; // ゴールデンイエロー
	case  4: return D2D1_COLOR_F{ 1.f, 1.f, 0.f, 1.f }; // 黄色
	case  5: return D2D1_COLOR_F{ 0.75f, 1.f, 0.f, 1.f }; // 明るい黄緑色 
	case  6: return D2D1_COLOR_F{ 0.f, 1.f, 0.f, 1.f }; // 緑
	case  7: return D2D1_COLOR_F{ 0.f, 1.f, 0.75f, 1.f }; // 黄緑色
	case  8: return D2D1_COLOR_F{ 0.f, 0.75f, 1.f, 1.f }; // セルリアンブルー
	case  9: return D2D1_COLOR_F{ 0.f, 0.3f, 1.f, 1.f }; // コバルトブルー
	case 10: return D2D1_COLOR_F{ 0.5f, 0.5f, 0.5f, 1.f }; // グレー ※通常ドラム
	case 11: return D2D1_COLOR_F{ 0.3f, 0.f, 1.f, 1.f }; // ヒヤシンス
	case 12: return D2D1_COLOR_F{ 0.5f, 0.f, 1.f, 1.f }; // バイオレット
	case 13: return D2D1_COLOR_F{ 0.75f, 0.f, 1.f, 1.f }; // ムラサキ
	case 14: return D2D1_COLOR_F{ 1.f, 0.f, 1.f, 1.f }; // マゼンタ
	case 15: return D2D1_COLOR_F{ 1.f, 0.f, 0.5f, 1.f }; // ルビーレッド
	case 16: return D2D1_COLOR_F{ 0.75f, 0.f, 0.3f, 1.f }; // カーマイン
	}
	std::unreachable();
};

static constexpr const wchar_t* state2text(LuathVoice::EnvelopeState state)
{
	switch(state) {
	case LuathVoice::EnvelopeState::Attack: return L"Attack";
	case LuathVoice::EnvelopeState::Hold:   return L"Hold";
	case LuathVoice::EnvelopeState::Decay:  return L"Decay";
	case LuathVoice::EnvelopeState::Fade:   return L"Fade";
	case LuathVoice::EnvelopeState::Release:return L"Release";
	case LuathVoice::EnvelopeState::Free:   return L"Free";
	default: return L"Unknown";
	}
};
static std::wstring freq2scale(float freq) {
	// MEMO 表示用。 ひとまず平均律で算出している
	//mCalculatedFreq = 440 * exp2((static_cast<float>(mNoteNo) + mPitchBend - 69.0f) / 12.0f);

	static constexpr std::array<const wchar_t*, 128> scales {
		L"C-1", L"C#-1", L"D-1", L"D#-1", L"E-1", L"F-1", L"F#-1", L"G-1", L"G#-1", L"A-1", L"A#-1", L"B-1",
			L"C0", L"C#0", L"D0", L"D#0", L"E0", L"F0", L"F#0", L"G0", L"G#0", L"A0", L"A#0", L"B0",
			L"C1", L"C#1", L"D1", L"D#1", L"E1", L"F1", L"F#1", L"G1", L"G#1", L"A1", L"A#1", L"B1",
			L"C2", L"C#2", L"D2", L"D#2", L"E2", L"F2", L"F#2", L"G2", L"G#2", L"A2", L"A#2", L"B2",
			L"C3", L"C#3", L"D3", L"D#3", L"E3", L"F3", L"F#3", L"G3", L"G#3", L"A3", L"A#3", L"B3",
			L"C4", L"C#4", L"D4", L"D#4", L"E4", L"F4", L"F#4", L"G4", L"G#4", L"A4", L"A#4", L"B4",
			L"C5", L"C#5", L"D5", L"D#5", L"E5", L"F5", L"F#5", L"G5", L"G#5", L"A5", L"A#5", L"B5",
			L"C6", L"C#6", L"D6", L"D#6", L"E6", L"F6", L"F#6", L"G6", L"G#6", L"A6", L"A#6", L"B6",
			L"C7", L"C#7", L"D7", L"D#7", L"E7", L"F7", L"F#7", L"G7", L"G#7", L"A7", L"A#7", L"B7",
			L"C8", L"C#8", L"D8", L"D#8", L"E8", L"F8", L"F#8", L"G8", L"G#8", L"A8", L"A#8", L"B8",
			L"C9", L"C#9", L"D9", L"D#9", L"E9", L"F9", L"F#9", L"G9",
	};

	auto rawNoteNo = log2(freq / 440.f) * 12.f + 69.f;
	auto roundedNoteNo = static_cast<int32_t>(rawNoteNo);

	if(roundedNoteNo < 0) {
		return L"<LOW>";
	}
	else if(roundedNoteNo > 128) {
		return L"<HIGH>";
	}
	else [[likely]] {
		return scales[roundedNoteNo];
	}
}
MainWindow::MainWindow()
	: mSequencer(*this)
	, mSynthesizer()
	, mOscilloScopeWidget()
	, mSpectrumAnalyzerWidget()
{
}

MainWindow::~MainWindow()
{
	dispose();

	// ウィンドウ破棄
	if (mWindowHandle) {
		DestroyWindow(mWindowHandle);
		mWindowHandle = nullptr;
	}
}
bool MainWindow::initialize()
{
	using namespace std::string_literals;

	// D2D描画関連初期化
	check(SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &mD2DFactory)));
	mFontLoader.createFontCollection(L"UmeFont"s, std::filesystem::current_path().append(L"assets/font/ume-tgo4.ttf"s));
	mDrawingContext = std::make_unique<DrawingContext>();

	// ウィンドウクラス生成
	static std::once_flag windowClassInitializeOnceFlag;
	std::call_once(windowClassInitializeOnceFlag, [] {
		WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = &wndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
		wcex.lpszClassName = L"luath_main_window";
		check(RegisterClassEx(&wcex) != 0);
	});

	// ウィンドウの生成
	// - Step1 : まずは生成されるディスプレイを特定するため、サイズ0で作成する
	auto instanceHandle = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
	mWindowHandle = CreateWindow(
		L"luath_main_window",
		L"luath - LibSynth++ Sample MIDI Synthesizer",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		nullptr,
		nullptr,
		instanceHandle,
		this);
	check(mWindowHandle != nullptr);

	// - Step2 : ウィンドウが所属するディスプレイが確定したため、このウィンドウのDPI値を用いてリサイズする
	onDpiChanged(static_cast<float>(GetDpiForWindow(mWindowHandle)) / 96.0f);
	ShowWindow(mWindowHandle, SW_SHOWNORMAL);
	UpdateWindow(mWindowHandle);

	// シーケンサセットアップ
	auto midi_path = std::filesystem::current_path();
	midi_path.append(L"assets/sample_midi/brambles_vsc3.mid"s); // 試験用MIDIファイル
	loadMidi(midi_path);
	
	// オーディオ周り初期化
	mAudioDeviceManager.initialiseWithDefaultDevices(
		0, // numInputChannelsNeeded
		2 // numOutputChannelsNeeded
	);
	mAudioDevice = mAudioDeviceManager.getCurrentAudioDevice();
	if(!mAudioDevice) {
		Log::w("Audio device is not found.");
	}
	mAudioDeviceManager.addAudioCallback(this);


	return true;
}
void MainWindow::dispose()
{
	// 再生停止
	mAudioDeviceManager.closeAudioDevice();
	mAudioDeviceManager.removeAudioCallback(this);

	// シーケンサ停止
	mSequencer.stop();

	// トーンジェネレータ停止
	mSynthesizer.dispose();
}
LRESULT MainWindow::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::optional<LRESULT> result;

	// 前処理 : 対象となるオブジェクトを解決できるようにプロパティをセットしておく
	if(message == WM_CREATE) {
		auto cs = reinterpret_cast<const CREATESTRUCT*>(lParam);
		SetProp(hWnd, L"this_ptr", reinterpret_cast<HANDLE>(cs->lpCreateParams));
	}

	// thisが解決できる物のみハンドリングする
	if(auto this_ = reinterpret_cast<MainWindow*>(GetProp(hWnd, L"this_ptr"))) {
		switch(message) {
		case WM_CREATE:
			// D&Dを受け入れ許可
			DragAcceptFiles(hWnd, TRUE);
			result = 0;
			break;
		case WM_KEYDOWN: {
			if(this_->onKeyDown(static_cast<uint16_t>(wParam), (GetKeyState(VK_SHIFT) & 0x8000), (GetKeyState(VK_CONTROL) & 0x8000), (GetKeyState(VK_MENU) & 0x8000))) {
				result = 0;
			}
		}	break;
		case WM_DROPFILES: {
			auto hDrop = reinterpret_cast<HDROP>(wParam);
			std::vector<std::filesystem::path> paths;
			UINT files = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
			for(size_t i = 0; i < files; ++i) {
				std::vector<TCHAR> buff;
				buff.resize(DragQueryFile(hDrop, static_cast<UINT>(i), nullptr, 0) + 1);
				if(DragQueryFile(hDrop, static_cast<UINT>(i), buff.data(), static_cast<UINT>(buff.size())) > 0) {
					buff.back() = _TEXT('\0');
					paths.emplace_back(buff.data());
				}
			}
			if(!paths.empty()) {
				this_->onDropFile(paths);
			}
		}	result = 0;
			break;
		case WM_DPICHANGED:
			this_->onDpiChanged(LOWORD(wParam) / 96.f);
			result = 0;
			break;
		case WM_PAINT:
			this_->onDraw();
			result = 0;
			break;
		case WM_DESTROY:
			juce::MessageManager::getInstance()->stopDispatchLoop();
			result = 0;
			break;
		}
	}

	// ハンドリングされなかったメッセージはデフォルトのウィンドウプロシージャに任せる
	if(!result) {
		result = DefWindowProc(hWnd, message, wParam, lParam);
	}

	// 後処理 : 定義されたプロパティ類は明示的に解放する必要がある
	if(message == WM_NCDESTROY) {
		EnumPropsEx(hWnd, [](HWND hWnd, LPTSTR lpszString, HANDLE, ULONG_PTR)->BOOL {
			RemoveProp(hWnd, lpszString);
			return TRUE;
		}, NULL);
	}

	return *result;
}

void MainWindow::onDropFile(const std::vector<std::filesystem::path>& paths)
{
	// シーケンサセットアップ
	if (paths.empty()) return;
	auto midi_path = std::filesystem::path(paths.front());
	loadMidi(midi_path);
}

bool MainWindow::onKeyDown(uint16_t key, bool shift, bool ctrl, bool alt)
{
	if (key == VK_UP) {
		mPostAmpVolume.store(mPostAmpVolume.load() * 1.5f);
		return true;
	} else if(key == VK_DOWN) {
		mPostAmpVolume.store(mPostAmpVolume.load() / 1.5f);
		return true;
	}
	return false;
}
void MainWindow::onDpiChanged(float scale)
{
	// ウィンドウの移動を実施
	SetWindowPos(
		mWindowHandle,
		nullptr,
		0,
		0,
		static_cast<int>(SCREEN_WIDTH * scale),
		static_cast<int>(SCREEN_HEIGHT * scale),
		SWP_NOMOVE
	);

	// 再描画を依頼
	UpdateWindow(mWindowHandle);
}
void MainWindow::loadMidi(const std::filesystem::path& path) {
	// 現在の再生を停止
	mSequencer.stop();

	// MIDIファイルをロード
	juce::FileInputStream midiInputStream(juce::File(path.wstring().c_str()));
	juce::MidiFile midiFile;
	if(!midiFile.readFrom(midiInputStream)) {
		// ロード失敗
		Log::e("Broken midi file : {}", path.string());
	}
	
	// 再生開始
	mSequencer.load(std::move(midiFile));
	mSequencer.start();
}

void MainWindow::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
	juce::ScopedLock sl(mMidiBufferLock);

	// TODO 本来 sampleNumber は次にレンダリングするMIDIメッセージが何サンプル目に解釈すべきかを示しているべき。
	//      smd::midi::Sequencer 側のあり方含めて検討すること
	int sampleNumber = 0;
	mMidiBuffer.addEvent(message, sampleNumber);
}

void MainWindow::audioDeviceIOCallbackWithContext(
	const float* const* inputChannelData,
	int numInputChannels,
	float* const* outputChannelData,
	int numOutputChannels,
	int numSamples,
	const juce::AudioIODeviceCallbackContext& context
)
{
	// 出力先の準備
	check(numOutputChannels == 2);
	check(numSamples >= 0);
	auto sig = lsp::Signal<float>::fromAudioBuffer(
		juce::AudioBuffer<float>(
			outputChannelData,	// 出力先バッファに直接書き込むようにする
			numOutputChannels,
			numSamples
		)
	);

	// シンセサイザ発音
	{
		juce::ScopedLock sl(mMidiBufferLock);
		mSynthesizer.renderNextBlock(sig.data(), mMidiBuffer, 0, numSamples);
		mMidiBuffer.clear();
	}

	// ポストエフェクト適用
	auto postAmpVolume = mPostAmpVolume.load();
	for(uint32_t ch = 0; ch < sig.channels(); ++ch) {
		auto data = sig.mutableData(ch);
		for(size_t i = 0; i < sig.samples(); ++i) {
			data[i] *= postAmpVolume;
		}
	}

	// UI側へ配送
	// TODO 信号生成に影響を与えにくい経路で配送したい
	mOscilloScopeWidget.write(sig);
	mSpectrumAnalyzerWidget.write(sig);
	mLissajousWidget.write(sig);
}
void MainWindow::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
	mAudioDevice = device;
	if(!device) return;

	auto sampleFreq = static_cast<float>(device->getCurrentSampleRate());
	mSynthesizer.setCurrentPlaybackSampleRate(device->getCurrentSampleRate());
	mOscilloScopeWidget.setParams(sampleFreq, 25e-3f);
	mSpectrumAnalyzerWidget.setParams(sampleFreq, 4096, 2);
	mLissajousWidget.setParams(sampleFreq, 25e-3f);
}
void MainWindow::audioDeviceStopped()
{
	// nop
}
void MainWindow::audioDeviceError(const juce::String& errorMessage)
{
	Log::e("Audio device error : {}", errorMessage.toStdString());
}

struct MainWindow::DrawingContext
{
	// FPS計算,表示用
	size_t frames = 0;
	std::array<std::chrono::microseconds, 15> drawing_time_history = {};
	std::array<std::chrono::microseconds, 15> frame_interval_history = {};
	clock::time_point prev_drawing_start_time = clock::now();
	size_t drawing_time_index = 0;
};


void MainWindow::onDraw()
{
	auto& context = *mDrawingContext;
	auto drawingScale = static_cast<float>(GetDpiForWindow(mWindowHandle)) / 96.f;

	// レンダリングターゲットの作成
	RECT windowRect;
	GetClientRect(mWindowHandle, &windowRect);

	auto renderTargetProperties = D2D1::RenderTargetProperties();
	renderTargetProperties.dpiX = drawingScale * 96.f;
	renderTargetProperties.dpiY = drawingScale * 96.f;

	auto renderHwndTargetProperties = D2D1::HwndRenderTargetProperties(mWindowHandle);
	renderHwndTargetProperties.pixelSize.width = static_cast<UINT32>(windowRect.right - windowRect.left);
	renderHwndTargetProperties.pixelSize.height = static_cast<UINT32>(windowRect.bottom - windowRect.top);
	renderHwndTargetProperties.presentOptions = D2D1_PRESENT_OPTIONS_NONE; // VSYNC(垂直同期)有効

	CComPtr<ID2D1HwndRenderTarget> renderTarget;
	check(SUCCEEDED(mD2DFactory->CreateHwndRenderTarget(
		renderTargetProperties,
		renderHwndTargetProperties,
		&renderTarget
	)));

	auto& renderer = *renderTarget;
	
	// 描画開始
	auto drawing_start_time = clock::now();
	context.frame_interval_history[context.drawing_time_index] = std::chrono::duration_cast<std::chrono::microseconds>(drawing_start_time - context.prev_drawing_start_time);
	context.prev_drawing_start_time = drawing_start_time;
	renderer.BeginDraw();

	renderer.SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
	renderer.Clear(D2D1::ColorF(D2D1::ColorF::White));
	
	// 描画メイン
	onDraw(renderer);

	// 描画終了
	auto drawing_end_time = clock::now();

	// ディスプレイへ反映 ※VSYNC待機
	renderer.EndDraw();
	context.drawing_time_history[context.drawing_time_index] = std::chrono::duration_cast<std::chrono::microseconds>(drawing_end_time - drawing_start_time);
	++context.drawing_time_index;
	if(context.drawing_time_index == context.drawing_time_history.size()) {
		context.drawing_time_index = 0;
	}
	++context.frames;
}
void MainWindow::onDraw(ID2D1RenderTarget& renderer)
{
	using namespace  std::string_literals;
	auto& context = *mDrawingContext;

	
	// 描画開始
	CComPtr<ID2D1SolidColorBrush> brush; // 汎用ブラシ
	check(SUCCEEDED(renderer.CreateSolidColorBrush({ 0.f, 0.f, 0.f, 1.f }, &brush)));
	CComPtr<ID2D1SolidColorBrush> blackBrush; // 色指定ブラシ : ■
	check(SUCCEEDED(renderer.CreateSolidColorBrush({ 0.f, 0.f, 0.f, 1.f }, &blackBrush)));
	renderer.Clear({ 1.f, 1.f, 1.f, 1.f });


	auto textFormatDefault = mFontLoader.createTextFormat(L"UmeFont"s, L"梅ゴシック"s, 12);
	auto textFormatSmall = mFontLoader.createTextFormat(L"UmeFont"s, L"梅ゴシック"s, 10);

	auto drawText = [&](float x, float y, std::wstring_view str) {
		renderer.DrawText(
			str.data(), static_cast<UINT32>(str.size()), textFormatDefault,
			{x, y, 65536.f, 65536.f},blackBrush
			);
	};
	auto drawSmallText = [&](float x, float y, std::wstring_view str) {
		renderer.DrawText(
			str.data(), static_cast<UINT32>(str.size()), textFormatSmall,
			{ x, y, 65536.f, 65536.f }, blackBrush
		);
	};

	// 各種情報取得
	const auto tgStatistics = mSynthesizer.statistics();
	const auto synthDigest = mSynthesizer.digest();
	const auto& channelDigests = synthDigest.channels;
	const auto& voiceDigests = synthDigest.voices;

	// チャネル毎の合計発音数を取得
	std::array<int, 16> poly;
	std::fill(poly.begin(), poly.end(), 0);
	for(auto& vd : voiceDigests) {
		if(vd.ch < 1 || vd.ch > 16) continue;
		if(vd.state == LuathVoice::EnvelopeState::Free) continue;
		++poly[vd.ch - 1];
	}
	int polyCount = std::accumulate(poly.begin(), poly.end(), 0);

	// 描画情報
	{
		auto average_interval_time = std::accumulate(context.frame_interval_history.cbegin(), context.frame_interval_history.cend(), std::chrono::microseconds(0)) / context.frame_interval_history.size();
		auto average_drawing_time = std::accumulate(context.drawing_time_history.cbegin(), context.drawing_time_history.cend(), std::chrono::microseconds(0)) / context.drawing_time_history.size();
		if(average_interval_time.count() > 0) {
			auto drawing_load = static_cast<float>(average_drawing_time.count()) / static_cast<float>(average_interval_time.count());
			drawText(0, 0, std::format(L"描画時間 : {:05.2f}[msec]", average_drawing_time.count() / 1000.f));
			drawText(0, 15, std::format(L"描画負荷 : {:06.2f}[%]", 100 * drawing_load));
		}

	}

	// 演奏情報
	{
		auto systemType = synthDigest.systemType.toPrintableWString();

		if(mAudioDevice != nullptr) {
			auto sampleFreq = mAudioDevice->getCurrentSampleRate();
			auto buffered = mAudioDevice->getCurrentBufferSizeSamples() / sampleFreq;
			auto latency = mAudioDevice->getOutputLatencyInSamples() / sampleFreq;

			drawText(150, 0, std::format(L"バッファ : {:04}[msec]  レイテンシ : {:04}[msec]  デバイス名 : {}  ",
				buffered * 1000,
				latency * 1000,
				mAudioDevice->getName().toWideCharPointer()
			));
		}

		drawText(150, 15, std::format(L"演奏負荷 : {:06.2f}[%]", 100 * mAudioDeviceManager.getCpuUsage()));
		drawText(0, 30, std::format(L"マスタ音量 : {:.3f}", synthDigest.masterVolume));
		drawText(150, 30, std::format(L"再生音量 : {:.3f}", mPostAmpVolume.load()));
		drawText(280, 15, std::format(L"同時発音数 : {:03}", polyCount));
		drawText(420, 15, std::format(L"MIDIリセット : {}", systemType));
	}

	// チャネル情報
	{
		const float ofsX = 10;
		const float ofsY = 60;
		const float height = 15;

		constexpr std::array<float, 10> columnWidth{ 25, 75, 40, 40, 60, 35, 75, 25, 25, 25 };
		float x = ofsX;
		size_t ci = 0;
		auto col = [&] {
			float ret = x;
			x += columnWidth[ci++];
			return ret;
		};
		float y = ofsY;
		constexpr float unscaled_width = std::accumulate(columnWidth.begin(), columnWidth.end(), 0.f);
		const float width = unscaled_width;

		{
			x = ofsX;
			ci = 0;
			drawText(col(), y, L"Ch");
			drawText(col(), y, L"Ins");
			drawText(col(), y, L"Vol");
			drawText(col(), y, L"Exp");
			drawText(col(), y, L"Pitch");
			drawText(col(), y, L"Pan");
			drawText(col(), y, L"Envelope");
			drawText(col(), y, L"Ped");
			drawText(col(), y, L"Drm");
			drawText(col(), y, L"Poly");
		}
		for(const auto& cd : channelDigests) {
			y += height;
			x = ofsX;
			ci = 0;

			const auto bgColor = getMidiChannelColor(cd.ch);
			brush->SetColor({ bgColor.r, bgColor.g, bgColor.b, bgColor.a / 2 });
			renderer.FillRectangle({ x, y, x + width , y + height }, brush);

			drawText(col(), y, std::format(L"{:02}", cd.ch));
			drawText(col(), y, std::format(L"{:03}:{:03}.{:03}", cd.progId, cd.bankSelectMSB, cd.bankSelectLSB));
			drawText(col(), y, std::format(L"{:0.3f}", cd.volume));
			drawText(col(), y, std::format(L"{:0.3f}", cd.expression));
			drawText(col(), y, std::format(L"{:+0.4f}", cd.pitchBend));
			drawText(col(), y, std::format(L"{:0.2f}", cd.pan));
			drawText(col(), y, std::format(L"{:03}.{:03}.{:03}",
				static_cast<int>(std::round(cd.attackTime * 127)),
				static_cast<int>(std::round(cd.decayTime * 127)),
				static_cast<int>(std::round(cd.releaseTime * 127))));
			drawText(col(), y, cd.pedal ? L"on" : L"off");
			drawText(col(), y, cd.drum ? L"on" : L"off");
			drawText(col(), y, std::format(L"{:02}", poly[cd.ch - 1]));
		}
	}

	// ボイス情報 (折り返し表示)
	{
		float ofsX = 460;
		const float ofsY = 60;
		const size_t voicePerRow = 45;
		const float height = 12;

		constexpr std::array<float, 4> columnWidth{ 15, 22, 30, 40};
		float x = ofsX;
		size_t ci = 0;
		auto col = [&] {
			float ret = x;
			x += columnWidth[ci++];
			return ret;
		};
		constexpr float unscaled_width = std::accumulate(columnWidth.begin(), columnWidth.end(), 0.f);
		const float width = unscaled_width;
		{
			drawText(col(), ofsY, L"Ch");
			drawText(col(), ofsY, L"Sc.");
			drawText(col(), ofsY, L"Env.");
			drawText(col(), ofsY, L"Status");
		}
		for(size_t i = 0; i < voiceDigests.size(); ++i) {
			const auto& vd = voiceDigests[i];
			if(vd.state == LuathVoice::EnvelopeState::Free) continue;

			x = ofsX + (static_cast<float>(i / voicePerRow)) * (width + 10);
			float y = ofsY + height * ((i % voicePerRow) + 1);
			ci = 0;

			const auto bgColor = getMidiChannelColor(vd.ch);
			brush->SetColor({ bgColor.r, bgColor.g, bgColor.b, std::clamp(vd.envelope * 0.4f + 0.1f, 0.f, 1.f)});
			renderer.FillRectangle({ x, y, x + width, y + height }, brush);

			drawSmallText(col(), y, std::format(L"{:02}", vd.ch));
			drawSmallText(col(), y, freq2scale(vd.freq));
			drawSmallText(col(), y, std::format(L"{:.3f}", vd.envelope));
			drawSmallText(col(), y, state2text(vd.state));
		}
	}



	// 波形情報
	{
		const float margin = 5; // unsecaledx
		mLissajousWidget.draw(
			renderer,
			10 + margin,
			340 + margin,
			150 - margin * 2,
			150 - margin * 2
		);
		mOscilloScopeWidget.draw(
			renderer,
			160 + margin,
			340 + margin,
			300 - margin * 2,
			150 - margin * 2
		);
		mSpectrumAnalyzerWidget.draw(
			renderer,
			10 + margin,
			490 + margin,
			450 - margin * 2,
			150 - margin * 2
		);
	}
}