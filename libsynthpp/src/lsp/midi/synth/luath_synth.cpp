/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/midi/synth/luath_synth.hpp>
#include <lsp/midi/messages/basic_message.hpp>
#include <lsp/midi/messages/sysex_message.hpp>

using namespace lsp::midi::synth;

LuathSynth::LuathSynth(uint32_t sampleFreq, midi::SystemType defaultSystemType)
	: mSampleFreq(sampleFreq)
{
	mMidiChannels.reserve(MAX_CHANNELS);
	for (uint8_t ch = 0; ch < MAX_CHANNELS; ++ch) {
		mMidiChannels.emplace_back(sampleFreq, ch, mWaveTable);
	}

	reset(defaultSystemType);
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
	mWaveTable.reset();

	for (auto& midich : mMidiChannels) {
		midich.reset(type);
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
		const auto& [msg_time, msg] = mMessageQueue.front();
		handleMidiEvent(msg);
		++msg_count;
		mMessageQueue.pop_front();
	}

	// 信号生成
	constexpr float MASTER_VOLUME = 0.125f;

	auto sig = lsp::Signal<float>::allocate(2, len);
	auto lData = sig.mutableData(0);
	auto rData = sig.mutableData(1);

	for (size_t i = 0; i < len; ++i) {
		auto& lch = lData[i];
		auto& rch = rData[i];
		lch = rch = 0;

		// チャネル毎の信号を生成する
		for (size_t ch = 0; ch < MAX_CHANNELS; ++ch) {
			auto& midich = mMidiChannels[ch];
			auto v = midich.update();
			lch += v.first;
			rch += v.second;
		}

		// マスタボリューム適用
		lch *= MASTER_VOLUME;
		rch *= MASTER_VOLUME;
	}

	// 演奏終了
	auto cycleEnd = clock::now();
	mStatistics.cycle_time = cycleEnd - cycleBegin;
	mThreadSafeStatistics = mStatistics;
	return sig;
}

// MIDIメッセージ受信コールバック
void LuathSynth::onMidiMessageReceived(clock::time_point msg_time, const juce::MidiMessage& msg)
{
	std::lock_guard lock(mMutex);
	mMessageQueue.emplace_back(msg_time, msg);
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

	digest.channels.reserve(mMidiChannels.size());
	for (auto& ch : mMidiChannels) {
		digest.channels.push_back(ch.digest());
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
	check(channel > 0);
	auto& midich = mMidiChannels[static_cast<size_t>(channel) -1];
	midich.noteOn(noteNo, velocity);
}
void LuathSynth::noteOff(int channel, int noteNo, float velocity, bool allowTailOff)
{
	// MEMO 一般に、MIDIではノートオフの代わりにvel=0のノートオンが使用されるため、呼ばれることは希である
	check(channel > 0);
	auto& midich = mMidiChannels[static_cast<size_t>(channel) - 1];
	midich.noteOff(noteNo, allowTailOff);
}
void LuathSynth::allNotesOff(int channel, bool allowTailOff)
{
	for(auto& midich : mMidiChannels) {
		midich.allNotesOff(allowTailOff);
	}
}
void LuathSynth::handleProgramChange(int channel, int progId)
{
	check(channel > 0);
	auto& midich = mMidiChannels[static_cast<size_t>(channel) - 1];
	midich.programChange(progId);
}

#if 0
void LuathSynth::dispatchMessage(const std::shared_ptr<const midi::Message>& msg)
{
	using namespace midi::messages;
		auto& midich = mMidiChannels[controlChange->channel()];
		midich.controlChange(controlChange->ctrlNo(), controlChange->value());
	} else if (auto pitchBend = std::dynamic_pointer_cast<const PitchBend>(msg)) {
		auto& midich = mMidiChannels[pitchBend->channel()];
		midich.pitchBend(pitchBend->pitch());
	} else if (auto sysEx = std::dynamic_pointer_cast<const SysExMessage>(msg)) {
		sysExMessage(&sysEx->data()[0], sysEx->data().size());
	}
}
#endif
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
			reset(midi::SystemType::GM1);
		}
		else if(match({ 0x7F, 0x09, 0x03 })) {
			// GM2 System On
			reset(midi::SystemType::GM2);
		}
		else if(match({ 0x7F, 0x09, 0x02 })) {
			// GM System Off → GS Reset
			reset(midi::SystemType::GS);
		}
	}
	else if(makerId == 0x41) {
		// Roland
		if(
			match({ {/*dev:any*/}, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41 }) || // GS Reset
			match({ {/*dev:any*/}, 0x42, 0x12, 0x00, 0x00, 0x7F, 0x00, 0x01 }) || // System Mode Set 1 // ※32パートで1音源
			match({ {/*dev:any*/}, 0x42, 0x12, 0x00, 0x00, 0x7F, 0x01, 0x00 })    // System Mode Set 2 // ※16パートを2音源扱いにする。現在ほぼ使われないとのこと
			) {
			reset(midi::SystemType::GS); // TODO System Mode Set 1/2に正確に対応する
		} else if(match({ {/*dev:any*/}, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41 })) {
				// GS Reset
				reset(midi::SystemType::GS);
			}
		else if(match({ {/*dev:any*/}, 0x42, 0x12, 0x40, {/*1x:part*/}, 0x15, {/*mn*/}}) && (*peek(4) & 0xF0) == 0x10) {
			// ドラムパート指定
			//   see https://ssw.co.jp/dtm/drums/drsetup.html
			auto ch = static_cast<uint8_t>(*peek(4) & 0x0F);
			auto mapNo = *peek(6);
			mMidiChannels[ch].setDrumMode(mapNo != 0);
		}
	}
	else if(makerId == 0x43) {
		// YAMAHA
		if(match({ {/*dev:any*/}, 0x4C, 0x00, 0x00, 0x7E, 0x00 })) {
			// XG Reset
			reset(midi::SystemType::XG);
		}
	}
}
