/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/midi/synth/luath_synth.hpp>
#include <lsp/generator/function_generator.hpp>

using namespace lsp::midi::synth;

LuathSynth::LuathSynth()
{

	// 波形テーブルのセットアップ
	mPresetWaveTable.loadPreset();
	{
		auto sig = Signal<float>::allocate(1);
		*sig.mutableData(0) = 0;
		mSquareWaveTable.add(0, std::move(sig), 0); // ground
	}
	for(uint32_t waveIndex = 1; waveIndex <= 50; ++waveIndex) {
		constexpr size_t samples = 512;
		auto sig = Signal<float>::allocate(samples);
		auto data = sig.mutableData(0);
		std::fill(data, data + samples, 0.f);
		for(uint32_t dim = 0; dim < waveIndex; ++dim) {
			generator::FunctionGenerator<float> fg;
			fg.setSinWave(static_cast<float>(samples), static_cast<float>(1 + dim * 2));
			for(size_t i = 0; i < samples; ++i) {
				data[i] += fg.update() / (1 + dim * 2);
			}
		}
		mSquareWaveTable.add(waveIndex, std::move(sig), 1.f);
	}

	// MIDIチャネルのセットアップ
	addSound(new ZeroSound());
	for(int ch = 1; ch <= 16; ++ch) {
		addSound(new ChannelSound(*this, ch));
	}
	check(dynamic_cast<ZeroSound*>(getSound(0).get()) != nullptr);
	for(int ch = 1; ch <= 16; ++ch) {
		auto sound = dynamic_cast<ChannelSound*>(getSound(ch).get());
		check(sound != nullptr);
		check(sound->midiChannel() == ch);
	}

	// 周波数 仮指定 ※間接的にMIDIリセット
	setSampleFreq(1.f);
}
// サンプ林周波数を指定します
void LuathSynth::setSampleFreq(float sampleFreq)
{
	std::lock_guard lock(mMutex);
	mSampleFreq = sampleFreq;

	// 最終段のローパスフィルタ ※折り返し誤差低減のため
	mFinalLpfL.setLopassParam(sampleFreq, sampleFreq / 3, 1.f);
	mFinalLpfR.setLopassParam(sampleFreq, sampleFreq / 3, 1.f);

	// 完全にリセットする
	reset();
}
LuathSynth::~LuathSynth()
{
	dispose();
}

void LuathSynth::dispose()
{
	std::lock_guard lock(mMutex);
}

// ---
// 各種パラメータ類 リセット
void LuathSynth::reset(midi::SystemType type)
{
	mSystemType = type;
	mMasterVolume = 1.f;

	for(int ch = 1; ch <= 16; ++ch) {
		getChannelSound(ch).reset(type);
	}
}


lsp::Signal<float> LuathSynth::generate(size_t len)
{
	auto cycleBegin = clock::now();

	// 演奏開始
	std::lock_guard lock(mMutex);

	// 蓄積されたMIDIメッセージを解釈
	size_t msg_count = 0;
	while(!mMessageQueue.empty()) {
		handleMidiEvent(mMessageQueue.front());
		++msg_count;
		mMessageQueue.pop_front();
	}

	// 信号生成
	constexpr float MASTER_ATTENUATOR = 0.125f;

	auto sig = lsp::Signal<float>::allocate(2, len);
	auto lData = sig.mutableData(0);
	auto rData = sig.mutableData(1);

	for (size_t i = 0; i < len; ++i) {
		auto& lch = lData[i];
		auto& rch = rData[i];
		lch = rch = 0;

		// チャネル毎の信号を生成する
		for (int ch = 1; ch <= 16; ++ch) {
			auto& midich = getChannelSound(ch);
			auto v = midich.update();
			lch += v.first;
			rch += v.second;
		}

		// 最終段用のローパスフィルタを適用
		lch = mFinalLpfL.update(lch);
		rch = mFinalLpfR.update(rch);

		// マスタボリューム適用
		lch *= mMasterVolume * MASTER_ATTENUATOR;
		rch *= mMasterVolume * MASTER_ATTENUATOR;
	}

	// 演奏終了
	auto cycleEnd = clock::now();
	mStatistics.cycle_time = cycleEnd - cycleBegin;
	mThreadSafeStatistics = mStatistics;
	return sig;
}

// MIDIメッセージ受信コールバック
void LuathSynth::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
	std::lock_guard lock(mMutex);
	mMessageQueue.emplace_back(message);
}
// 統計情報を取得します
LuathSynth::Statistics LuathSynth::statistics()const
{
	return mThreadSafeStatistics;
}
LuathSynth::Digest LuathSynth::digest()const
{
	std::shared_lock lock(mMutex);
	Digest digest;
	digest.systemType = mSystemType;
	digest.masterVolume = mMasterVolume;

	digest.channels.reserve(16);
	for(int ch = 1; ch <= 16; ++ch) {
		digest.channels.push_back(getChannelSound(ch).digest());
	}
	return digest;
}

// ---


void LuathSynth::handleMidiEvent(const juce::MidiMessage& msg)
{
	if(msg.isSysEx()) {
		sysExMessage(static_cast<const uint8_t*>(msg.getSysExData()), static_cast<size_t>(msg.getSysExDataSize()));
	}
	else {
		juce::Synthesiser::handleMidiEvent(msg);
	}
}


void LuathSynth::noteOn(int channel, int noteNo, float velocity)
{
	getChannelSound(channel).noteOn(noteNo, velocity);
}
void LuathSynth::noteOff(int channel, int noteNo, float velocity, bool allowTailOff)
{
	// MEMO 一般に、MIDIではノートオフの代わりにvel=0のノートオンが使用されるため、呼ばれることは希である
	getChannelSound(channel).noteOff(noteNo, allowTailOff);
}
void LuathSynth::allNotesOff(int channel, bool allowTailOff)
{
	getChannelSound(channel).allNotesOff(allowTailOff);
}
void LuathSynth::handlePitchWheel(int channel, int value)
{
	getChannelSound(channel).pitchBend(value);
}
void LuathSynth::handleController(int channel, int ctrlNo, int value)
{
	getChannelSound(channel).controlChange(ctrlNo, value);
}
void LuathSynth::handleProgramChange(int channel, int progId)
{
	getChannelSound(channel).programChange(progId);
}

// ---


// システムエクスクルーシブ
void LuathSynth::sysExMessage(const uint8_t* data, size_t len)
{
	size_t pos = 0;
	auto peek = [&](size_t offset = 0) -> std::optional<uint8_t> {
		if(pos + offset >= len) return {};
		return data[pos + offset];
	};
	auto read = [&]() -> std::optional<uint8_t> {
		if(pos >= len) return {};
		return data[pos++];
	};
	auto match = [&](const std::vector<std::optional<uint8_t>>& pattern) -> bool {
		for(size_t i = 0; i < pattern.size(); ++i) {
			auto v = peek(i);
			if(!v.has_value()) return false;
			if(pattern[i].has_value() && pattern[i].value() != v.value()) return false;
		}
		return true;
	};

	auto makerId = read();
	if(makerId == 0x7E) {
		// リアルタイム ユニバーサルシステムエクスクルーシブ
		// https://www.g200kg.com/jp/docs/tech/universalsysex.html

		if(match({ 0x7F, 0x09, 0x01 })) {
			// GM1 System On
			reset(midi::SystemType::GM1());
		}
		else if(match({ 0x7F, 0x09, 0x03 })) {
			// GM2 System On
			reset(midi::SystemType::GM2());
		}
		else if(match({ 0x7F, 0x09, 0x02 })) {
			// GM System Off → GS Reset
			reset(midi::SystemType::GS());
		}
		else if(match({ 0x7F, {/*dev:any*/}, 0x04, 0x01, 0x00, {/*volume*/} })) {
			// マスターボリューム
			mMasterVolume = *peek(5) / 127.f;
		}
	}
	else if(makerId == 0x41) {
		// Roland
		if(match({ {/*dev:any*/}, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41 })) {
			// GS Reset
			reset(midi::SystemType::GS());
		} else if(match({ {/*dev:any*/}, 0x42, 0x12, 0x00, 0x00, 0x7F, 0x00, 0x01 })) {
			// System Mode Set 1 // ※32パートで1音源
			reset(midi::SystemType::SystemModeSet1()); 
		} else if(match({ {/*dev:any*/}, 0x42, 0x12, 0x00, 0x00, 0x7F, 0x01, 0x00 })) {
			// System Mode Set 2 // ※16パートを2音源扱いにする。現在ほぼ使われないとのこと
			reset(midi::SystemType::SystemModeSet2());
		}
		else if(match({ {/*dev:any*/}, 0x42, 0x12, 0x40, {/*1x:part*/}, 0x15, {/*mn*/}}) && (*peek(4) & 0xF0) == 0x10) {
			// ドラムパート指定
			//   see https://ssw.co.jp/dtm/drums/drsetup.html
			auto ch = static_cast<int>(*peek(4) & 0x0F) + 1;
			auto mapNo = *peek(6);
			getChannelSound(ch).setDrumMode(mapNo != 0);
		}
	}
	else if(makerId == 0x43) {
		// YAMAHA
		if(match({ {/*dev:any*/}, 0x4C, 0x00, 0x00, 0x7E, 0x00 })) {
			// XG Reset
			reset(midi::SystemType::XG());
		}
	}
}
