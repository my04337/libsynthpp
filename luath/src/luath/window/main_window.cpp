#include <luath/window/main_window.hpp>
#include <luath/app/application.hpp>
#include <lsp/midi/smf/parser.hpp>

#include <shellapi.h>

#include <array>
#include <map>
#include <map>
#include <numeric>
#include <fstream>

using namespace lsp;
using namespace luath;
using namespace luath::window;

using lsp::synth::Voice;
using lsp::synth::VoiceId;

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 680;
static constexpr uint32_t SAMPLE_FREQ = 44100;

static constexpr std::array<D2D1_COLOR_F, 16> CHANNEL_COLOR{
	D2D1_COLOR_F{ 1.f, 0.f, 0.f, 1.f}, // 赤
	D2D1_COLOR_F{ 1.f, 0.5f, 0.f, 1.f }, // 朱色
	D2D1_COLOR_F{ 1.f, 0.75f, 0.f, 1.f }, // ゴールデンイエロー
	D2D1_COLOR_F{ 1.f, 1.f, 0.f, 1.f }, // 黄色
	D2D1_COLOR_F{ 0.75f, 1.f, 0.f, 1.f }, // 明るい黄緑色 
	D2D1_COLOR_F{ 0.f, 1.f, 0.f, 1.f }, // 緑
	D2D1_COLOR_F{ 0.f, 1.f, 0.75f, 1.f }, // 黄緑色
	D2D1_COLOR_F{ 0.f, 0.75f, 1.f, 1.f }, // セルリアンブルー
	D2D1_COLOR_F{ 0.f, 0.3f, 1.f, 1.f }, // コバルトブルー
	D2D1_COLOR_F{ 0.5f, 0.5f, 0.5f, 1.f }, // グレー ※通常ドラム
	D2D1_COLOR_F{ 0.3f, 0.f, 1.f, 1.f }, // ヒヤシンス
	D2D1_COLOR_F{ 0.5f, 0.f, 1.f, 1.f }, // バイオレット
	D2D1_COLOR_F{ 0.75f, 0.f, 1.f, 1.f }, // ムラサキ
	D2D1_COLOR_F{ 1.f, 0.f, 1.f, 1.f }, // マゼンタ
	D2D1_COLOR_F{ 1.f, 0.f, 0.5f, 1.f }, // ルビーレッド
	D2D1_COLOR_F{ 0.75f, 0.f, 0.3f, 1.f }, // カーマイン
};

static constexpr const wchar_t* state2text(Voice::EnvelopeState state)
{
	switch(state) {
	case Voice::EnvelopeState::Attack: return L"Attack";
	case Voice::EnvelopeState::Hold:   return L"Hold";
	case Voice::EnvelopeState::Decay:  return L"Decay";
	case Voice::EnvelopeState::Fade:   return L"Fade";
	case Voice::EnvelopeState::Release:return L"Release";
	case Voice::EnvelopeState::Free:   return L"Free";
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
	else if(roundedNoteNo >= 128) {
		return L"<HIGH>";
	}
	else [[likely]] {
		return scales[roundedNoteNo];
	}
}
MainWindow::MainWindow()
	: mInstrumentTable(InstrumentLoader::loadFromDirectory(std::filesystem::current_path() / L"assets/instruments"))
	, mSequencer(mSynthesizer)
	, mSynthesizer(SAMPLE_FREQ, mInstrumentTable)
	, mOutput()
	, mLissajousWidget(SAMPLE_FREQ, static_cast<uint32_t>(SAMPLE_FREQ * 250e-4f))
	, mOscilloScopeWidget(SAMPLE_FREQ, static_cast<uint32_t>(SAMPLE_FREQ * 250e-4f))
	, mSpectrumAnalyzerWidget(SAMPLE_FREQ, 4096)
{
	mSynthesizer.setRenderingCallback([this](Signal<float>&& sig){onRenderedSignal(std::move(sig));});

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
void MainWindow::dispose()
{
	// 再生停止
	auto isOutputStopped = mOutput.stop();

	// シーケンサ停止
	mSequencer.stop();

	// トーンジェネレータ停止
	mSynthesizer.dispose();
}
bool MainWindow::initialize()
{
	using namespace std::string_literals;

	// D2D描画関連初期化
	lsp_check(SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &mD2DFactory)));
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
		lsp_check(RegisterClassEx(&wcex) != 0);
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
	lsp_check(mWindowHandle != nullptr);

	// - Step2 : ウィンドウが所属するディスプレイが確定したため、このウィンドウのDPI値を用いてリサイズする
	onDpiChanged(static_cast<float>(GetDpiForWindow(mWindowHandle)) / 96.0f);
	ShowWindow(mWindowHandle, SW_SHOWNORMAL);
	UpdateWindow(mWindowHandle);

	// シーケンサセットアップ
	auto midi_path = std::filesystem::current_path();
	midi_path.append(L"assets/sample_midi/brambles_vsc3.mid"s); // 試験用MIDIファイル
	loadMidi(midi_path);

	// オーディオ出力 初期化
	if(!mOutput.start()) {
		MessageBox(
			mWindowHandle,
			L"出力デバイスのオープンに失敗しました。",
			L"オーディオ エラー",
			MB_OK | MB_ICONWARNING
		);
	}
	return true;
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
			auto fin_act_dragFinish = finally([hDrop]{ DragFinish(hDrop); });
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
			ValidateRect(hWnd, nullptr);
			this_->onDraw();
			InvalidateRect(hWnd, nullptr, FALSE);
			result = 0;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
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
	// シーケンサのキー操作
	if (key == VK_UP) {
		// 出力音量を上げる ※TOCTOUは無視可能。この操作がメインスレッド以外から行われることはない。
		mPostAmpVolume.store(mPostAmpVolume.load() * 1.5f);
		return true;
	} else if(key == VK_DOWN) {
		// 出力音量を下げる ※TOCTOUは無視可能。この操作がメインスレッド以外から行われることはない。
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
void MainWindow::loadMidi(const std::filesystem::path& midi_path) {
	mSequencer.stop();
	mSequencer.reset(midi::SystemType::GM1());

#ifdef NDEBUG
	try {
#endif
		auto parsed = midi::smf::Parser::parse(midi_path);
		mSequencer.load(std::move(parsed.second));
		mSequencer.start();
#ifdef NDEBUG
	} catch (const midi::smf::decoding_exception& e) {
		// ロード失敗
		std::string_view detail = e.what();
		std::wstring detailW(detail.begin(), detail.end());
		MessageBox(
			mWindowHandle,
			std::format(L"MIDIファイルを開けません : {}\n\n{}", midi_path.wstring(), detailW).c_str(),
			L"ファイルエラー",
			MB_OK | MB_ICONWARNING | MB_SETFOREGROUND
		);
	}
#endif
}

struct MainWindow::DrawingContext
{
	// FPS計算,表示用
	size_t frames = 0;
	std::array<std::chrono::microseconds, 15> drawing_time_history = {};
	size_t drawing_time_index = 0;

	// 前回のボイス表示位置 (素直に順番に描画するとちらついて見づらいため表示を揃える)
	std::unordered_map<VoiceId, size_t> prevVoicePosMap;
	size_t prevVoiceEndPos = 0;
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
	renderHwndTargetProperties.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;

	CComPtr<ID2D1HwndRenderTarget> renderTarget;
	lsp_check(SUCCEEDED(mD2DFactory->CreateHwndRenderTarget(
		renderTargetProperties,
		renderHwndTargetProperties,
		&renderTarget
	)));

	auto& renderer = *renderTarget;
	
	// 描画開始
	auto drawing_start_time = clock::now();
	renderer.BeginDraw();

	renderer.SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
	renderer.Clear(D2D1::ColorF(D2D1::ColorF::White));
	
	// 描画メイン
	onDraw(renderer);


	// 描画終了
	renderer.EndDraw();
	auto drawing_end_time = clock::now();

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
	lsp_check(SUCCEEDED(renderer.CreateSolidColorBrush({ 0.f, 0.f, 0.f, 1.f }, &brush)));
	CComPtr<ID2D1SolidColorBrush> blackBrush; // 色指定ブラシ : ■
	lsp_check(SUCCEEDED(renderer.CreateSolidColorBrush({ 0.f, 0.f, 0.f, 1.f }, &blackBrush)));
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

	size_t polyCount = 0;
	for(auto& cd : channelDigests) polyCount += cd.poly;

	// 描画情報
	{
		drawText(0, 0, std::format(L"描画フレーム数 : {}", context.frames));
		auto average_time = std::accumulate(context.drawing_time_history.cbegin(), context.drawing_time_history.cend(), std::chrono::microseconds(0)) / context.drawing_time_history.size();
		drawText(0, 15, std::format(L"描画時間 : {:0.2f}[msec]", average_time.count()/1000.f));
	}

	// 演奏情報
	{
		const wchar_t* systemType = synthDigest.systemType.toPrintableWString();

		drawText(150, 0, std::format(L"生成時間 : {}[msec]  failed : {}[msec]  buffered : {:04}[msec]",
			tgStatistics.created_samples * 1000ull / SAMPLE_FREQ,
			tgStatistics.failed_samples * 1000ull / SAMPLE_FREQ,
			mOutput.getBufferedFrameCount() * 1000 / SAMPLE_FREQ
		));
		drawText(150, 15, std::format(L"演奏負荷 : {:03}[%]", (int)(100 * tgStatistics.rendering_load_average())));
		drawText(150, 30, std::format(L"PostAmp : {:.3f}", mPostAmpVolume.load()));
		drawText(280, 15, std::format(L"同時発音数 : {:03}", polyCount));
		drawText(420, 15, std::format(L"MIDIリセット : {}", systemType));
	}

	// チャネル情報
	{
		const float ofsX = 10;
		const float ofsY = 60;
		const float height = 15;

		constexpr std::array<float, 11> columnWidth{ 25, 75, 40, 40, 45, 35, 75, 25, 25, 25 };
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

			const auto bgColor = CHANNEL_COLOR[cd.ch];
			brush->SetColor({ bgColor.r, bgColor.g, bgColor.b, bgColor.a / 2 });
			renderer.FillRectangle({ x, y, x + width , y + height }, brush);

			drawText(col(), y, std::format(L"{:02}", cd.ch));
			drawText(col(), y, std::format(L"{:03}:{:03}.{:03}", cd.progId, cd.bankSelectMSB, cd.bankSelectLSB));
			drawText(col(), y, std::format(L"{:0.3f}", cd.volume));
			drawText(col(), y, std::format(L"{:0.3f}", cd.expression));
			drawText(col(), y, std::format(L"{:+0.3f}", cd.pitchBend));
			drawText(col(), y, std::format(L"{:0.2f}", cd.pan));
			drawText(col(), y, std::format(L"{:03}.{:03}.{:03}", cd.attackTime, cd.decayTime, cd.releaseTime));
			drawText(col(), y, cd.pedal ? L"on" : L"off");
			drawText(col(), y, cd.drum ? L"on" : L"off");
			drawText(col(), y, std::format(L"{:02}", cd.poly));
		}
	}

	// ボイス情報 (折り返し表示)
	{
		float ofsX = 460;
		const float ofsY = 60;
		const size_t voicePerRow = 45;
		const float height = 12;

		// 全チャネルのボイス情報を統合
		std::unordered_map<VoiceId, std::pair</*ch*/uint8_t, Voice::Digest>> unsortedVoiceDigests;
		for(const auto& cd : channelDigests) {
			for(const auto& [vid, vd] : cd.voices) {
				unsortedVoiceDigests.emplace(vid, std::make_pair(cd.ch, vd));
			}
		}
		// 前回の描画位置を維持しながら描画順を決定する
		std::vector<std::tuple<VoiceId, /*ch*/uint8_t, Voice::Digest>> voiceDigests;
		voiceDigests.resize(std::max(unsortedVoiceDigests.size(), context.prevVoiceEndPos));
		for(auto& [vid, pos] : context.prevVoicePosMap) {
			auto found = unsortedVoiceDigests.find(vid);
			if(found != unsortedVoiceDigests.end()) {
				voiceDigests[pos] = std::make_tuple(found->first, std::get<0>(found->second), std::get<1>(found->second));
				unsortedVoiceDigests.erase(found);
			}
		}
		for(size_t i = 0; i < voiceDigests.size() && !unsortedVoiceDigests.empty(); ++i) {
			if(!std::get<0>(voiceDigests[i]).empty()) continue;
			auto found = unsortedVoiceDigests.begin();
			voiceDigests[i] = std::make_tuple(found->first, std::get<0>(found->second), std::get<1>(found->second));
			unsortedVoiceDigests.erase(found);
		}
		lsp_check(unsortedVoiceDigests.empty());
		context.prevVoiceEndPos = 0;
		context.prevVoicePosMap.clear();
		for(size_t i = 0; i < voiceDigests.size(); ++i) {
			if(std::get<0>(voiceDigests[i]).empty()) continue;
			context.prevVoicePosMap[std::get<0>(voiceDigests[i])] = i;
			context.prevVoiceEndPos = i + 1;
		}

		// 描画
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
			const auto& [vid, ch, vd] = voiceDigests[i];
			x = ofsX + (static_cast<float>(i / voicePerRow)) * (width + 10);
			float y = ofsY + height * ((i % voicePerRow) + 1);
			ci = 0;

			if(vid.empty()) continue;

			const auto bgColor = CHANNEL_COLOR[ch];
			brush->SetColor({ bgColor.r, bgColor.g, bgColor.b, std::clamp(vd.envelope * 0.4f + 0.1f, 0.f, 1.f)});
			renderer.FillRectangle({ x, y, x + width, y + height }, brush);

			drawSmallText(col(), y, std::format(L"{:02}", ch));
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
void MainWindow::onRenderedSignal(Signal<float>&& sig)
{
	// ポストアンプ適用
	auto postAmpVolume = mPostAmpVolume.load();
	size_t samples = sig.frames() * sig.channels();
	auto p = sig.data();
	for (size_t i = 0; i < samples; ++i) {
		p[i] *= postAmpVolume;
	}

	// 各出力先に配送
	mOutput.write(sig);
	mOscilloScopeWidget.write(sig);
	mSpectrumAnalyzerWidget.write(sig);
	mLissajousWidget.write(sig);
}