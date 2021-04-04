#include <LSP/Synth/Luath.hpp>
#include <LSP/MIDI/Messages/BasicMessage.hpp>
#include <LSP/MIDI/Messages/SysExMessage.hpp>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::MIDI::Messages;
using namespace LSP::Synth;

Luath::Luath(uint32_t sampleFreq, SystemType defaultSystemType)
	: mSampleFreq(sampleFreq)
	, mPlayingThreadAborted(false)
{
	mMidiChannels.reserve(MAX_CHANNELS);
	for (uint8_t ch = 0; ch < MAX_CHANNELS; ++ch) {
		mMidiChannels.emplace_back(sampleFreq, ch);
	}

	reset(defaultSystemType);

	mPlayingThread = std::thread([this]{playingThreadMain();});
}
Luath::~Luath()
{
	dispose();
}

void Luath::dispose()
{
	std::lock_guard lock(mMutex);

	// 全タスク停止
	mTaskDispatcher.abort();

	// コールバック破棄
	mRenderingCallback = nullptr;

	// 演奏スレッド停止
	mPlayingThreadAborted = true;
	if (mPlayingThread.joinable()) {
		mPlayingThread.join();
	}
}

// ---

void Luath::playingThreadMain()
{
	constexpr auto RENDERING_INTERVAL = std::chrono::milliseconds(10);

	uint64_t SAMPLES_LIMIT_PER_RENDERING = 10*uint64_t(std::chrono::duration_cast<std::chrono::duration<double>>(RENDERING_INTERVAL).count() * mSampleFreq);
	
	// 演奏ループ開始
	const clock::time_point begin_time = clock::now() - RENDERING_INTERVAL;

	clock::time_point prev_wake_up_time = begin_time;
	uint64_t prev_sample_pos = 0;
	while (true) {
		auto cycleBegin = clock::now();
		auto next_wake_up_time = prev_wake_up_time;
		while(next_wake_up_time < clock::now()) next_wake_up_time += RENDERING_INTERVAL;
		std::this_thread::sleep_until(next_wake_up_time);
		uint64_t next_sample_pos = std::chrono::duration_cast<std::chrono::microseconds>(next_wake_up_time-begin_time).count() * (uint64_t)mSampleFreq / 1000000ull;
		uint64_t need_samples = next_sample_pos - prev_sample_pos;
		prev_wake_up_time = next_wake_up_time;
		prev_sample_pos = next_sample_pos;

		uint64_t make_samples = std::min(need_samples, SAMPLES_LIMIT_PER_RENDERING);

		if(mPlayingThreadAborted) break;

		// 演奏開始
		std::lock_guard lock(mMutex);


		// 指定時刻時点までに蓄積されたMIDIメッセージを解釈
		size_t msg_count = 0;
		while (!mMessageQueue.empty()) {
			const auto& [msg_time, msg] = mMessageQueue.front();
			if(msg_time >= prev_wake_up_time) break;
			dispatchMessage(msg);
			++msg_count;
			mMessageQueue.pop_front();
		}
	
		// 信号生成
		auto beginRendering = clock::now();
		auto sig = generate(make_samples);
		auto endRendering = clock::now();
		mStatistics.rendering_time = endRendering - beginRendering;
		mStatistics.created_samples += sig.frames();
		mStatistics.failed_samples += (need_samples - make_samples);

		if(mRenderingCallback) mRenderingCallback(std::move(sig));
		
		// 演奏終了
		auto cycleEnd = clock::now();
		mStatistics.cycle_time = cycleEnd - cycleBegin;
		mThreadSafeStatistics = mStatistics;
	}
}
// ---
// 各種パラメータ類 リセット
void Luath::reset(SystemType type)
{
	mSystemType = type;

	for (auto& midich : mMidiChannels) {
		midich.reset(type);
	}
}


LSP::Signal<float> Luath::generate(size_t len)
{
	constexpr float MASTER_VOLUME = 0.125f;

	auto sig = LSP::Signal<float>::allocate(&mMem, 2, len);

	for (size_t i = 0; i < len; ++i) {
		auto frame = sig.frame(i);
		frame[0] = frame[1] = 0;

		// Tone, Channel
		for (size_t ch = 0; ch < MAX_CHANNELS; ++ch) {
			auto& midich = mMidiChannels[ch];
			auto v = midich.update();
			frame[0] += v.first;
			frame[1] += v.second;
		}

		// Master
		frame[0] *= MASTER_VOLUME;
		frame[1] *= MASTER_VOLUME;
	}
	return sig;
}

// MIDIメッセージ受信コールバック
void Luath::onMidiMessageReceived(clock::time_point msg_time, const std::shared_ptr<const MIDI::Message>& msg)
{
	std::lock_guard lock(mMutex);
	mMessageQueue.emplace_back(msg_time, msg);
}
// 音声が生成された際のコールバック関数を設定します
void Luath::setRenderingCallback(RenderingCallback cb)
{
	std::lock_guard lock(mMutex);
	mRenderingCallback = std::move(cb);
}
// 統計情報を取得します
Luath::Statistics Luath::statistics()const
{
	return mThreadSafeStatistics;
}
void Luath::dispatchMessage(const std::shared_ptr<const MIDI::Message>& msg)
{
	if (auto m = std::dynamic_pointer_cast<const NoteOn>(msg)) {
		noteOn(m->channel(), m->noteNo(), m->velocity());
	} else if (auto m = std::dynamic_pointer_cast<const NoteOff>(msg)) {
		noteOff(m->channel(), m->noteNo());
	} else if (auto m = std::dynamic_pointer_cast<const ControlChange>(msg)) {
		controlChange(m->channel(), m->ctrlNo(), m->value());
	} else if (auto m = std::dynamic_pointer_cast<const SysExMessage>(msg)) {
		sysExMessage(&m->data()[0], m->data().size());
	}
}
// ---
// ノートオン
void Luath::noteOn(uint8_t ch, uint8_t noteNo, uint8_t vel)
{
	if(ch >= mMidiChannels.size()) return;
	auto& midich = mMidiChannels[ch];

	midich.noteOn(noteNo, vel);
}

// ノートオフ
void Luath::noteOff(uint8_t ch, uint8_t noteNo)
{
	// MEMO 一般に、MIDIではノートオフの代わりにvel=0のノートオンが使用されるため、呼ばれることは希である

	if(ch >= mMidiChannels.size()) return;
	auto& midich = mMidiChannels[ch];

	midich.noteOff(noteNo);
}

// コントロールチェンジ
void Luath::controlChange(uint8_t ch, uint8_t ctrlNo, uint8_t value)
{
	// 参考 : http://quelque.sakura.ne.jp/midi_cc.html
	
	if(ch >= mMidiChannels.size()) return;
	auto& midich = mMidiChannels[ch];
	
	bool apply_RPN_NRPN_state = false;

	switch (ctrlNo) {
	case 6: // Data Entry(MSB)
		midich.ccDE_MSB = value;
		midich.ccDE_LSB.reset();
		apply_RPN_NRPN_state = true; // MSBのみでよいものはこのタイミングで適用する
		break;
	case 10: // Pan(パン)
		midich.ccPan = (value / 127.0f);
		break;
	case 11: // Expression(エクスプレッション)
		midich.ccExpression = (value / 127.0f);
		break;
	case 36: // Data Entry(LSB)
		midich.ccDE_LSB = value;
		apply_RPN_NRPN_state = true; // MSBのみでよいものはこのタイミングで適用する
		break;
	case 64: // Hold1(ホールド1:ダンパーペダル)
		if(value < 0x64) {
			midich.holdOff();
		} else {
			midich.holdOn();
		}
		break;
	case 98: // NRPN(LSB)
		midich.ccNRPN_LSB = value;
		break;
	case 99: // NRPN(MSB)
		midich.resetParameterNumberState();
		midich.ccNRPN_MSB = value;
		break;
	case 100: // RPN(LSB)
		midich.ccRPN_LSB = value;
		break;
	case 101: // RPN(MSB)
		midich.resetParameterNumberState();
		midich.ccRPN_MSB = value;
		break;
	}

	// RPN/NRPNの受付が禁止されている場合、適用しない
	if (midich.rpnNull) {
		apply_RPN_NRPN_state = false;
	}

	// RPN/NRPN 適用
	if (apply_RPN_NRPN_state) {
		if (midich.ccRPN_MSB == 0 && midich.ccRPN_MSB == 0 && midich.ccDE_MSB.has_value()) {
			// ピッチベンドセンシティビティ: MSBのみ使用
			midich.rpnPitchBendSensitibity = midich.ccDE_MSB.value();
		}
	}

	midich.ccPrevCtrlNo = ctrlNo;
	midich.ccPrevValue = value;
}

// システムエクスクルーシブ
void Luath::sysExMessage(const uint8_t* data, size_t len)
{
	size_t pos = 0;
	auto peek = [&](size_t offset = 0) -> std::optional<uint8_t> {
		if(pos+offset >= len) return {};
		return data[pos+offset];
	};
	auto read = [&]() -> std::optional<uint8_t> {
		if(pos >= len) return {};
		return data[pos++];
	};
	auto match = [&](const std::vector<std::optional<uint8_t>>& pattern) -> bool {
		for (size_t i = 0; i < pattern.size(); ++i) {
			auto v = peek(i);
			if(!v.has_value()) return false;
			if(pattern[i].has_value() && pattern[i].value() != v.value()) return false;
		}
		return true;
	};

	auto makerId = read();
	if (makerId == 0x7E) {
		// リアルタイム ユニバーサルシステムエクスクルーシブ
		// https://www.g200kg.com/jp/docs/tech/universalsysex.html

		if (match({0x7F, 0x09, 0x01})) {
			// GM1 System On
			reset(SystemType::GM1);
		} 
		if(match({0x7F, 0x09, 0x03})) {
			// GM2 System On
			reset(SystemType::GM2);
		}
		if (match({0x7F, 0x09, 0x02})) {
			// GM System Off → GS Reset
			reset(SystemType::GS);
		}
	} 
	if (makerId = 0x41) {
		// Roland
		if (match({{/*dev:any*/}, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41})) {
			// GS Reset
			reset(SystemType::GS);
		}
	}
}
