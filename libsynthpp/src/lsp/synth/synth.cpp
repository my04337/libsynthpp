/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/synth/synth.hpp>
#include <lsp/synth/sound.hpp>
#include <lsp/synth/instruments.hpp>
#include <lsp/dsp/function_generator.hpp>

using namespace lsp::synth;

static constexpr int MAX_VOICE_COUNT = 90;

LuathSynth::LuathSynth()
{
	// 波形テーブルを先にロードしておく
	Instruments::prepareWaveTable();

	// MIDIチャネルのセットアップ
	mChannelState.reserve(16);
	for(int ch = 1; ch <= 16; ++ch) {
		mChannelState.emplace_back(*this, ch);
		addSound(new LuathSound(*this, ch));
	}
	check(getNumSounds() == 16);

	// ボイスのセットアップ
	for(int id = 0; id < MAX_VOICE_COUNT; ++id) {
		addVoice(new LuathVoice(*this));
	}
	check(getNumVoices() == MAX_VOICE_COUNT);


	// 周波数 仮指定 ※間接的にMIDIリセット
	setCurrentPlaybackSampleRate(1.f);
}
// サンプ林周波数を指定します
void LuathSynth::setCurrentPlaybackSampleRate(double sampleRate)
{
	juce::ScopedLock sl(lock);
	SUPER::setCurrentPlaybackSampleRate(sampleRate);
	auto sampleFreq = static_cast<float>(sampleRate);
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
	juce::ScopedLock sl(lock);
}

// ---
// 各種パラメータ類 リセット
void LuathSynth::reset(midi::SystemType type)
{
	mSystemType = type;
	mMasterVolume = 1.f;

	for(int ch = 1; ch <= 16; ++ch) {
		getChannelState(ch).reset();
		allNotesOff(ch, false);
	}
}
ChannelState& LuathSynth::getChannelState(int ch)noexcept 
{
	require(ch >= 1 && ch <= 16);
	return mChannelState[static_cast<size_t>(ch - 1)];
}
const ChannelState& LuathSynth::getChannelState(int ch)const noexcept
{
	require(ch >= 1 && ch <= 16);
	return mChannelState[static_cast<size_t>(ch - 1)];
}


// 信号を生成します
void LuathSynth::renderNextBlock(juce::AudioBuffer<float>& outputAudio, const juce::MidiBuffer& inputMidi, int startSample, int numSamples)
{
	auto cycleBegin = clock::now();

	require(outputAudio.getNumChannels() == 2);

	// 演奏開始
	juce::ScopedLock sl(lock);

	// 出力をゼロクリア
	outputAudio.clear(startSample, numSamples);

	// 各voiceの出力を実施
	SUPER::renderNextBlock(outputAudio, inputMidi, startSample, numSamples);

	// 後段のエフェクト類を適用
	constexpr float MASTER_ATTENUATOR = 0.075f;
	auto lch = outputAudio.getWritePointer(0, startSample);
	auto rch = outputAudio.getWritePointer(1, startSample);
	for(int i = 0; i < numSamples; ++i) {
		auto l = lch[i];
		auto r = rch[i];
		// 最終段用のローパスフィルタを適用
		l = mFinalLpfL.update(l);
		r = mFinalLpfR.update(r);

		// マスタボリューム適用
		l *= mMasterVolume * MASTER_ATTENUATOR;
		r *= mMasterVolume * MASTER_ATTENUATOR;

		// 書き戻し
		lch[i] = l;
		rch[i] = r;
	}

	// 演奏終了
	auto cycleEnd = clock::now();
	mStatistics.cycle_time = cycleEnd - cycleBegin;
	mThreadSafeStatistics = mStatistics;
}
// 統計情報を取得します
LuathSynth::Statistics LuathSynth::statistics()const
{
	return mThreadSafeStatistics;
}
LuathSynth::Digest LuathSynth::digest()const
{
	juce::ScopedLock sl(lock);
	Digest digest;
	digest.systemType = mSystemType;
	digest.masterVolume = mMasterVolume;

	digest.channels.reserve(16);
	for(int ch = 1; ch <= 16; ++ch) {
		digest.channels.push_back(getChannelState(ch).digest(mSystemType));
	}
	auto numVoice = getNumVoices();
	digest.voices.reserve(numVoice);
	for(int i = 0; i < numVoice; ++i) {
		auto voice = dynamic_cast<LuathVoice*>(getVoice(i));
		check(voice != nullptr);
		digest.voices.emplace_back(voice->digest());
	}

	return digest;
}
lsp::midi::SystemType LuathSynth::systemType()const noexcept
{
	return mSystemType;
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


void LuathSynth::handlePitchWheel(int channel, int value)
{
	auto pitchBendWithoutSensitivity = (value - 8192.f) / 8192.f;
	getChannelState(channel).pitchBendWithoutSensitivity = (value - 8192.f) / 8192.f;
	SUPER::handlePitchWheel(channel, value);
}
void LuathSynth::handleController(int channel, int ctrlNo, int value)
{
	getChannelState(channel).handleController(ctrlNo, value);
	SUPER::handleController(channel, ctrlNo, value);
}
void LuathSynth::handleProgramChange(int channel, int progId)
{
	getChannelState(channel).progId = progId;
	SUPER::handleProgramChange(channel, progId);
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
			getChannelState(ch).isDrumPart = (mapNo != 0);
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
