#include <Luath/Window/MainWindow.hpp>
#include <Luath/App/Application.hpp>
#include <Luath/App/FontCache.hpp>
#include <LSP/MIDI/Parser.hpp>

#include <SDL_ttf.h>
#include <array>
#include <numeric>

using namespace LSP;
using namespace Luath;
using namespace Luath::Window;

static constexpr uint32_t SAMPLE_FREQ = 44100;


MainWindow::MainWindow()
	: mDrawingThreadAborted(false)
	, mSynthesizer(SAMPLE_FREQ)
	, mSequencer(mSynthesizer)
#ifdef USE_WASAPI_OUTPUT
	, mOutput()
#else
	, mOutput(SAMPLE_FREQ, 2, Audio::SDLOutput::SampleFormat::Float32)
#endif
{
	lsp_assert(mOutput.start());
	mSynthesizer.setRenderingCallback([this](LSP::Signal<float>&& sig){onRenderedSignal(std::move(sig));});

	mOscilloScopeWidget.setParam(SAMPLE_FREQ, 2, SAMPLE_FREQ * 250e-4);
	mLissajousWidget.setParam(SAMPLE_FREQ, 2, SAMPLE_FREQ * 250e-4);
	mSpectrumAnalyzerWidget.setParam(SAMPLE_FREQ, 2, 4096);
}

MainWindow::~MainWindow()
{
	dispose();

	// ウィンドウ破棄
	if (mWindow) {
		SDL_DestroyWindow(mWindow);
	}
}
void MainWindow::dispose() 
{
	// 再生停止
	mOutput.stop();

	// シーケンサ停止
	mSequencer.stop();

	// トーンジェネレータ停止
	mSynthesizer.dispose();

	// 描画スレッド停止
	mDrawingThreadAborted = true;
	if (mDrawingThread.joinable()) {
		mDrawingThread.join();
	}
}
bool MainWindow::initialize()
{
	constexpr int SCREEN_WIDTH = 800;
	constexpr int SCREEN_HEIGHT = 640;

	// ウィンドウ生成
	auto window = SDL_CreateWindow(
		"luath - LibSynth++ Sample MIDI Synthesizer",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_OPENGL
	);
	if(!window) {
		Log::e(LOGF("Main : could not create window :" << SDL_GetError()));
		return false;
	}
	auto fail_act_destroy = finally([&]{SDL_DestroyWindow(window);});

	// シーケンサセットアップ
	auto midi_path = std::filesystem::current_path();
	midi_path.append("assets/sample_midi/brambles_vsc3.mid"); // 試験用MIDIファイル
	loadMidi(midi_path);

	// OK
	mWindow = window;
	fail_act_destroy.reset();
	mDrawingThread = std::thread([this]{drawingThreadMain();});
	return true;
}
void MainWindow::onDropFile(const SDL_DropEvent& ev)
{
	// シーケンサセットアップ
	if (!ev.file) return;
	auto midi_path = std::filesystem::u8path(ev.file);
	loadMidi(midi_path);
}
void MainWindow::onKeyDown(const SDL_KeyboardEvent& ev)
{
	if (ev.keysym.sym == SDLK_UP) {
		mPostAmpVolume.store(mPostAmpVolume.load() * 1.5f);
	} else if(ev.keysym.sym == SDLK_DOWN) {
		mPostAmpVolume.store(mPostAmpVolume.load() / 1.5f);
	}
}
void MainWindow::loadMidi(const std::filesystem::path& midi_path) {
	mSequencer.stop();
	mSequencer.reset(MIDI::SystemType::GM1);

	try {
		auto parsed = MIDI::Parser::parse(midi_path);
		mSequencer.load(std::move(parsed.second));
		mSequencer.start();
	} catch (const MIDI::decoding_exception& e) {
		// ロード失敗
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			reinterpret_cast<const char*>(u8"ファイルエラー"),
			reinterpret_cast<const char*>((u8"MIDIファイルを開けません : " + midi_path.u8string()).c_str()),
			mWindow
		);
	}

}
void MainWindow::drawingThreadMain()
{
	constexpr int FRAMES_PER_SECOND = 60;
	constexpr std::chrono::microseconds FRAME_INTERVAL(1'000'000/FRAMES_PER_SECOND);
	auto& app = Application::instance();

	constexpr SDL_Color COLOR_BLACK{0x00, 0x00, 0x00, 0xFF};

	// 描画用フォント取得
	auto default_font = app.fontCache().get(12);

	// レンダラ生成
	auto renderer = SDL_CreateRenderer(mWindow, -1, 0);
	lsp_assert(renderer != nullptr);
	auto fin_act_destroy_renderer = finally([&]{SDL_DestroyRenderer(renderer);});
	FastTextRenderer textRenderer(renderer, default_font);

	// FPS計算,表示用
	size_t frames = 0;
	std::array<std::chrono::microseconds, FRAMES_PER_SECOND/4> drawing_time_history = {};
	size_t drawing_time_index = 0;

	// 文字列プール ※SDL2_ttfはフォントを用いての描画からのテクスチャ生成までが非常に重いため、キャッシュが必要

	// 描画ループ開始
	clock::time_point prev_wake_up_time = clock::now() - FRAME_INTERVAL;
	while (true) {
		auto next_wake_up_time = prev_wake_up_time;
		while(next_wake_up_time < clock::now()) next_wake_up_time += FRAME_INTERVAL;
		std::this_thread::sleep_until(next_wake_up_time);
		prev_wake_up_time = next_wake_up_time;

		if(mDrawingThreadAborted) break;
		// 描画開始
		std::lock_guard lock(mDrawingMutex);
		auto drawing_start_time = clock::now();
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);

		// 描画情報
		{
			textRenderer.draw(0, 0, FORMAT_STRING(L"描画フレーム数 : " << frames));
			auto average_time = std::accumulate(drawing_time_history.cbegin(), drawing_time_history.cend(), std::chrono::microseconds(0)) / drawing_time_history.size();
			auto load_average = (int)(100.0 * average_time.count() / FRAME_INTERVAL.count());
			textRenderer.draw(0, 15, FORMAT_STRING(L"描画負荷 : " << std::setfill(L'0') << std::right << std::setw(3) << load_average << L"[%]"));
		}
		
		// 演奏情報
		{
			auto tgStatistics = mSynthesizer.statistics();
			textRenderer.draw(150, 0, FORMAT_STRING(L"生成サンプル数 : " << tgStatistics.created_samples << L" (" << (tgStatistics.created_samples * 1000ull / SAMPLE_FREQ)
				<< L"[msec])  failed : " << tgStatistics.failed_samples * 1000ull / SAMPLE_FREQ
				<< L"[msec]  buffered : " << std::setfill(L'0') << std::right << std::setw(4) << mOutput.getBufferedFrameCount() * 1000 / SAMPLE_FREQ << "[msec]"));
			textRenderer.draw(150, 15, FORMAT_STRING(L"演奏負荷 : " << std::setfill(L'0') << std::right << std::setw(3) << (int)(100 * tgStatistics.rendering_load_average()) << L"[%]"));
			textRenderer.draw(150, 30, FORMAT_STRING(L"PostAmp : " << std::fixed << std::setprecision(3) << mPostAmpVolume.load()));
		}

		// チャネル情報
		{
			constexpr int ofsX = 300;
			constexpr int ofsY = 30;

			constexpr std::array<int, 5> columnWidth{ 20, 55, 50, 50 };
			int x = ofsX, ci = 0;
			auto col = [&] { 
				int ret = x;  
				x += columnWidth[ci++];
				return ret;
			};
			int y = ofsY;
			{
				x = ofsX;
				ci = 0;
				textRenderer.draw(col(), y, L"Ch");
				textRenderer.draw(col(), y, L"Ins");
				textRenderer.draw(col(), y, L"Vol");
				textRenderer.draw(col(), y, L"Exp");
			}
			for (const auto& info : mSynthesizer.channelInfo()) {
				y += 15;
				x = ofsX;
				ci = 0;
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::setw(2) << info.ch));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::setw(3) << info.programChange << L"-" << std::setw(3) << info.bankSelect));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::fixed << std::setprecision(4) << info.volume));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::fixed << std::setprecision(4) << info.expression));

			}

		}
		


		// 波形情報
		const int margin = 5;
		mLissajousWidget.draw(renderer, 350+margin, 490+margin, 150-margin*2, 150-margin*2);
		mSpectrumAnalyzerWidget.draw(renderer, 500+margin, 340+margin, 300-margin*2, 150-margin*2);
		mOscilloScopeWidget.draw(renderer, 500+margin, 490+margin, 300-margin*2, 150-margin*2);

		// 描画終了
		SDL_RenderPresent(renderer);
		auto drawing_end_time = clock::now();
		drawing_time_history[drawing_time_index] = std::chrono::duration_cast<std::chrono::microseconds>(drawing_end_time - drawing_start_time);
		++drawing_time_index;;
		if (drawing_time_index == drawing_time_history.size()) {
			drawing_time_index = 0;
		}
		++frames;
	}
}
void MainWindow::onRenderedSignal(LSP::Signal<float>&& sig)
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