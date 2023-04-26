#include <Luath/Window/MainWindow.hpp>
#include <Luath/App/Application.hpp>
#include <Luath/App/FontCache.hpp>
#include <LSP/MIDI/Parser.hpp>

#include <SDL_ttf.h>
#include <array>
#include <map>
#include <numeric>

using namespace LSP;
using namespace Luath;
using namespace Luath::Window;

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 640;
static constexpr uint32_t SAMPLE_FREQ = 44100;

static constexpr std::array<SDL_Color, 16> CHANNEL_COLOR{
	SDL_Color{0xFF, 0x00, 0x00, 0xFF}, // 赤
	SDL_Color{0xFF, 0x80, 0x00, 0xFF}, // 朱色
	SDL_Color{0xFF, 0xBF, 0x00, 0xFF}, // ゴールデンイエロー
	SDL_Color{0xFF, 0xFF, 0x00, 0xFF}, // 黄色
	SDL_Color{0xBF, 0xFF, 0x00, 0xFF}, // 明るい黄緑色 
	SDL_Color{0x00, 0xFF, 0x00, 0xFF}, // 緑
	SDL_Color{0x00, 0xFF, 0xBF, 0xFF}, // 黄緑色
	SDL_Color{0x00, 0xBF, 0xFF, 0xFF}, // セルリアンブルー
	SDL_Color{0x00, 0x40, 0xFF, 0xFF}, // コバルトブルー
	SDL_Color{0x80, 0x80, 0x80, 0xFF}, // グレー ※通常ドラム
	SDL_Color{0x40, 0x00, 0xFF, 0xFF}, // ヒヤシンス
	SDL_Color{0x80, 0x00, 0xFF, 0xFF}, // バイオレット
	SDL_Color{0xBF, 0x00, 0xFF, 0xFF}, // ムラサキ
	SDL_Color{0xFF, 0x00, 0xFF, 0xFF}, // マゼンタ
	SDL_Color{0xFF, 0x00, 0x80, 0xFF}, // ルビーレッド
	SDL_Color{0xBF, 0x00, 0x40, 0xFF}, // カーマイン
};


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
	// 初期DPIセット : プライマリディスプレイのDPIを使用する
	float current_dpi = 96.0f;
	if(SDL_GetDisplayDPI(0, nullptr, nullptr, &current_dpi) == 0) {
		onDpiChanged(current_dpi / 96.0f);
	}

	// ウィンドウ生成
	auto window = SDL_CreateWindow(
		"luath - LibSynth++ Sample MIDI Synthesizer",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH * mDrawingScale, SCREEN_HEIGHT * mDrawingScale,
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

	// オーディオ出力 初期化
	if(!mOutput.start()) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_WARNING,
			reinterpret_cast<const char*>(u8"オーディオ エラー"),
			reinterpret_cast<const char*>(u8"出力デバイスのオープンに失敗しました。"),
			window
			);
	}
	// 生成OK
	mWindow = window;
	fail_act_destroy.reset();


	// 描画開始
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
	switch(ev.keysym.sym) {
	case SDLK_UP:		
		// ボリュームUp
		mPostAmpVolume.store(mPostAmpVolume.load() * 1.5f);
		break;
	case SDLK_DOWN:	
		// ボリュームDown
		mPostAmpVolume.store(mPostAmpVolume.load() / 1.5f);
		break;
	case SDLK_SPACE:
		// 一時停止/再開
		if(mSequencer.isPlaying()) {
			if(mSequencer.isPausing()) {
				mSequencer.resume();
			} else {
				mSequencer.pause();
			}
		}
		break;
	}
}
void MainWindow::onDpiChanged(float scale)
{
	mDrawingScale = scale;
	SDL_SetWindowSize(mWindow, SCREEN_WIDTH * scale, SCREEN_HEIGHT * scale);
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
	using LSP::Synth::VoiceId;

	constexpr int FRAMES_PER_SECOND = 60;
	constexpr std::chrono::microseconds FRAME_INTERVAL(1'000'000/FRAMES_PER_SECOND);
	auto& app = Application::instance();

	constexpr SDL_Color COLOR_BLACK{0x00, 0x00, 0x00, 0xFF};

	// 描画スケール (動的に変更される事に注意
	float s = 0.0f;

	// レンダラ生成
	auto renderer = SDL_CreateRenderer(mWindow, -1, 0);
	lsp_assert(renderer != nullptr);
	auto fin_act_destroy_renderer = finally([&]{SDL_DestroyRenderer(renderer);});
	FastTextRenderer textRenderer;
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// FPS計算,表示用
	size_t frames = 0;
	std::array<std::chrono::microseconds, FRAMES_PER_SECOND/4> drawing_time_history = {};
	size_t drawing_time_index = 0;

	// 前回のボイス表示位置 (素直に順番に描画するとちらついて見づらいため表示を揃える)
	std::unordered_map<VoiceId, int> prevVoicePosMap;
	size_t prevVoiceEndPos = 0;

	// 描画ループ開始
	clock::time_point prev_wake_up_time = clock::now() - FRAME_INTERVAL;
	while(true) {
		auto next_wake_up_time = prev_wake_up_time;
		while(next_wake_up_time < clock::now()) next_wake_up_time += FRAME_INTERVAL;
		std::this_thread::sleep_until(next_wake_up_time);
		prev_wake_up_time = next_wake_up_time;

		if(mDrawingThreadAborted) break;

		// 描画スケール再計算処理
		if(auto new_scale = mDrawingScale.load(); s != new_scale) {
			s = new_scale;

			auto default_font = app.fontCache().get(12 * s);
			textRenderer = FastTextRenderer(renderer, default_font);
		}

		// 描画開始
		std::lock_guard lock(mDrawingMutex);
		auto drawing_start_time = clock::now();
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);

		// 各種情報取得
		const auto tgStatistics = mSynthesizer.statistics();
		const auto synthDigest = mSynthesizer.digest();
		const auto& channelDigests = synthDigest.channels;

		int polyCount = 0;
		for(auto& cd : channelDigests) polyCount += cd.poly;


		// 描画情報
		{
			textRenderer.draw(0, 0, FORMAT_STRING(L"描画フレーム数 : " << frames));
			auto average_time = std::accumulate(drawing_time_history.cbegin(), drawing_time_history.cend(), std::chrono::microseconds(0)) / drawing_time_history.size();
			auto load_average = (int)(100.0 * average_time.count() / FRAME_INTERVAL.count());
			textRenderer.draw(0, 15 * s, FORMAT_STRING(L"描画負荷 : " << std::setfill(L'0') << std::right << std::setw(3) << load_average << L"[%]"));
		}

		// 演奏情報
		{
			const wchar_t* systemType = L"";
			switch(synthDigest.systemType) {
			case LSP::MIDI::SystemType::GM1:	systemType = L"GM1";	break;
			case LSP::MIDI::SystemType::GM2:	systemType = L"GM2";	break;
			case LSP::MIDI::SystemType::GS:		systemType = L"GS";		break;
			}

			textRenderer.draw(150 * s, 0, FORMAT_STRING(L"生成サンプル数 : " << tgStatistics.created_samples << L" (" << (tgStatistics.created_samples * 1000ull / SAMPLE_FREQ)
				<< L"[msec])  failed : " << tgStatistics.failed_samples * 1000ull / SAMPLE_FREQ
				<< L"[msec]  buffered : " << std::setfill(L'0') << std::right << std::setw(4) << mOutput.getBufferedFrameCount() * 1000 / SAMPLE_FREQ << "[msec]"));
			textRenderer.draw(150 * s, 15 * s, FORMAT_STRING(L"演奏負荷 : " << std::setfill(L'0') << std::right << std::setw(3) << (int)(100 * tgStatistics.rendering_load_average()) << L"[%]"));
			textRenderer.draw(150 * s, 30 * s, FORMAT_STRING(L"PostAmp : " << std::fixed << std::setprecision(3) << mPostAmpVolume.load()));
			textRenderer.draw(280 * s, 15 * s, FORMAT_STRING(L"同時発音数 : " << std::setfill(L'0') << std::right << std::setw(2) << polyCount));
			textRenderer.draw(420 * s, 15 * s, FORMAT_STRING(L"MIDIリセット : " << std::setfill(L'0') << systemType));
		}

		// チャネル情報
		{
			const float ofsX = 300 * s;
			const float ofsY = 50 * s;

			constexpr std::array<int, 10> columnWidth{ 25, 75, 40, 40, 60, 35, 75, 25, 25, 25 };
			float x = ofsX, ci = 0;
			auto col = [&] {
				int ret = x;
				x += columnWidth[ci++] * s;
				return ret;
			};
			int y = ofsY;
			constexpr float unscaled_width = std::accumulate(columnWidth.begin(), columnWidth.end(), 0);
			const float width = unscaled_width * s;
			const float height = 15 * s;

			{
				x = ofsX;
				ci = 0;
				textRenderer.draw(col(), y, L"Ch");
				textRenderer.draw(col(), y, L"Ins");
				textRenderer.draw(col(), y, L"Vol");
				textRenderer.draw(col(), y, L"Exp");
				textRenderer.draw(col(), y, L"Pitch");
				textRenderer.draw(col(), y, L"Pan");
				textRenderer.draw(col(), y, L"Envelope");
				textRenderer.draw(col(), y, L"Ped");
				textRenderer.draw(col(), y, L"Drm");
				textRenderer.draw(col(), y, L"Poly");
			}
			for(const auto& cd : channelDigests) {
				y += 15 * s;
				x = ofsX;
				ci = 0;

				const auto bgColor = CHANNEL_COLOR[cd.ch];
				SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a / 2);
				SDL_FRect bgRect{ x, y, width, height };
				SDL_RenderFillRectF(renderer, &bgRect);

				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::setw(2) << cd.ch));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::setw(3) << cd.progId << L":" << std::setw(3) << cd.bankSelectMSB << L"." << std::setw(3) << cd.bankSelectLSB));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::fixed << std::setprecision(3) << cd.volume));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::fixed << std::setprecision(3) << cd.expression));
				textRenderer.draw(col(), y, FORMAT_STRING((cd.pitchBend >= 0 ? L"+" : L"-") << std::fixed << std::setprecision(4) << std::abs(cd.pitchBend)));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::fixed << std::setprecision(2) << cd.pan));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::setw(3) << cd.attackTime << L"." << std::setw(3) << cd.decayTime << L"." << std::setw(3) << cd.releaseTime));
				textRenderer.draw(col(), y, FORMAT_STRING((cd.pedal ? L"on" : L"off")));
				textRenderer.draw(col(), y, FORMAT_STRING((cd.drum ? L"on" : L"off")));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::setw(2) << cd.poly));
			}
		}

		// ボイス情報
		{
			const int ofsX = 10 * s;
			const int ofsY = 50 * s;

			// 全チャネルのボイス情報を統合
			std::unordered_map<VoiceId, std::pair</*ch*/uint8_t, LSP::Synth::Voice::Digest>> unsortedVoiceDigests;
			for(const auto& cd : channelDigests) {
				for(const auto& [vid, vd] : cd.voices) {
					unsortedVoiceDigests.emplace(vid, std::make_pair(cd.ch, vd));
				}
			}
			// 前回の描画位置を維持しながら描画順を決定する
			std::vector<std::tuple<VoiceId, /*ch*/uint8_t, LSP::Synth::Voice::Digest>> voiceDigests;
			voiceDigests.resize(std::max(unsortedVoiceDigests.size(), prevVoiceEndPos));
			for(auto& [vid, pos] : prevVoicePosMap) {
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
			lsp_assert(unsortedVoiceDigests.empty());
			prevVoiceEndPos = 0;
			prevVoicePosMap.clear();
			for(size_t i = 0; i < voiceDigests.size(); ++i) {
				if(std::get<0>(voiceDigests[i]).empty()) continue;
				prevVoicePosMap[std::get<0>(voiceDigests[i])] = i;
				prevVoiceEndPos = i + 1;
			}

			// 描画
			constexpr std::array<int, 5> columnWidth{ 60, 25, 60, 50, 40 };
			int x = ofsX, ci = 0;
			auto col = [&] {
				int ret = x;
				x += columnWidth[ci++] * s;
				return ret;
			};
			constexpr auto state2text = [](LSP::Synth::Voice::EnvelopeState state) -> const wchar_t* {
				switch(state) {
				case LSP::Synth::Voice::EnvelopeState::Attack: return L"Attack";
				case LSP::Synth::Voice::EnvelopeState::Hold:   return L"Hold";
				case LSP::Synth::Voice::EnvelopeState::Decay:  return L"Decay";
				case LSP::Synth::Voice::EnvelopeState::Fade:   return L"Fade";
				case LSP::Synth::Voice::EnvelopeState::Release:return L"Release";
				case LSP::Synth::Voice::EnvelopeState::Free:   return L"Free";
				default: return L"Unknown";
				}
			};
			int y = ofsY;
			constexpr int unscaled_width = std::accumulate(columnWidth.begin(), columnWidth.end(), 0);
			const int width = unscaled_width * s;
			const int height = 15 * s;
			{
				x = ofsX;
				ci = 0;
				textRenderer.draw(col(), y, L"VoiceID");
				textRenderer.draw(col(), y, L"Ch");
				textRenderer.draw(col(), y, L"Freq");
				textRenderer.draw(col(), y, L"Envelope");
				textRenderer.draw(col(), y, L"State");
			}
			for(const auto& [vid, ch, vd] : voiceDigests) {
				y += 15 * s;
				x = ofsX;
				ci = 0;

				if(vid.empty()) continue;

				const auto bgColor = CHANNEL_COLOR[ch];
				SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, static_cast<Uint8>(bgColor.a * 0.5f * vd.envelope));
				SDL_Rect bgRect{ x, y, width, height };
				SDL_RenderFillRect(renderer, &bgRect);

				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::setw(8) << vid.id()));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::setw(2) << ch));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::fixed << std::setprecision(4) << vd.freq));
				textRenderer.draw(col(), y, FORMAT_STRING(std::setfill(L'0') << std::fixed << std::setprecision(3) << vd.envelope));
				textRenderer.draw(col(), y, FORMAT_STRING(state2text(vd.state)));
			}
		}



		// 波形情報
		{
			const int margin = 5; // unsecaled
			mLissajousWidget.draw(renderer, (350 + margin)*s, (340 + margin)* s, (150 - margin * 2)* s, (150 - margin * 2)* s);
			mOscilloScopeWidget.draw(renderer, (500 + margin)* s, (340 + margin)* s, (300 - margin * 2)* s, (150 - margin * 2)* s);
			mSpectrumAnalyzerWidget.draw(renderer, (350 + margin)* s, (490 + margin)* s, (450 - margin * 2)* s, (150 - margin * 2)* s);
		}

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