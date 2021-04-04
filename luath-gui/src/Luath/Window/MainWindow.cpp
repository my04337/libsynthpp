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
	, mOutput(SAMPLE_FREQ, 2, Audio::SDLOutput::SampleFormat::Float32)
{
	lsp_assert(mOutput.start());
	mSynthesizer.setRenderingCallback([this](LSP::Signal<float>&& sig){onRenderedSignal(std::move(sig));});

	mOscilloScope.setParam(SAMPLE_FREQ, 2, SAMPLE_FREQ*250e-4);
}

MainWindow::~MainWindow()
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

	// ウィンドウ破棄
	if (mWindow) {
		SDL_DestroyWindow(mWindow);
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
	midi_path.append("assets/midi/brambles_vsc3.mid"); // 試験用MIDIファイル
	auto parsed = MIDI::Parser::parse(midi_path);
	mSequencer.load(std::move(parsed.second));

	// OK
	mWindow = window;
	fail_act_destroy.reset();
	mDrawingThread = std::thread([this]{drawingThreadMain();});
	mSequencer.start();
	return true;
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

	// FPS計算,表示用
	size_t frames = 0;
	Text text_drawing_laod_average;
	std::array<std::chrono::microseconds, FRAMES_PER_SECOND/4> drawing_time_history = {};
	size_t drawing_time_index = drawing_time_history.size();


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
		auto text_frames = Text::make(renderer, default_font, FORMAT_STRING(L"描画フレーム数 : " << frames).c_str(), COLOR_BLACK);
		SDL_RenderCopy(renderer, text_frames, nullptr, &text_frames.rect(0, 0));
		if (drawing_time_index == drawing_time_history.size()) {
			auto average_time = std::accumulate(drawing_time_history.cbegin(), drawing_time_history.cend(), std::chrono::microseconds(0)) / drawing_time_history.size();
			auto load_average = (int)(100.0*average_time.count()/FRAME_INTERVAL.count());
			text_drawing_laod_average = Text::make(renderer, default_font, FORMAT_STRING(L"描画負荷 : " << std::setfill(L'0') << std::right << std::setw(3) << load_average << L"[%]").c_str(), COLOR_BLACK);
			drawing_time_index = 0;
		}		
		SDL_RenderCopy(renderer, text_drawing_laod_average, nullptr, &text_drawing_laod_average.rect(0, 15));
		
		// 演奏情報
		auto tgStatistics = mSynthesizer.statistics();
		auto text_samples = Text::make(renderer, default_font, FORMAT_STRING(L"生成サンプル数 : " << tgStatistics.created_samples << L" (" << (tgStatistics.created_samples*1000ull/SAMPLE_FREQ)
			<< L"[msec])  skipped : " << tgStatistics.skipped_samples*1000ull/SAMPLE_FREQ 
			<< L"[msec] buffered : " << std::setfill(L'0') << std::right << std::setw(4) << mOutput.getBufferedFrameCount()*1000 / SAMPLE_FREQ << "[msec]").c_str(), COLOR_BLACK);
		SDL_RenderCopy(renderer, text_samples, nullptr, &text_samples.rect(150, 0));
		auto text_rendering_laod_average = Text::make(renderer, default_font, FORMAT_STRING(L"演奏負荷 : " << std::setfill(L'0') << std::right << std::setw(3) << (int)(100*tgStatistics.rendering_load_average()) << L"[%]").c_str(), COLOR_BLACK);
		SDL_RenderCopy(renderer, text_rendering_laod_average, nullptr, &text_rendering_laod_average.rect(150, 15));
		
		// オシロスコープ
		mOscilloScope.draw(renderer, 500, 400, 300, 240);

		// 描画終了
		SDL_RenderPresent(renderer);
		auto drawing_end_time = clock::now();
		drawing_time_history[drawing_time_index] = std::chrono::duration_cast<std::chrono::microseconds>(drawing_end_time - drawing_start_time);
		++drawing_time_index;
		++frames;
	}
}
void MainWindow::onRenderedSignal(LSP::Signal<float>&& sig)
{
	mOutput.write(sig);
	mOscilloScope.write(sig);
}