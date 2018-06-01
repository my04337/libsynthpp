#include <Luath/Syntesizer/ToneGenerator.hpp>

using namespace LSP;
using namespace Luath::Synthesizer;

ToneGenerator::ToneGenerator(uint32_t sampleFreq, SystemType defaultSystemType)
{
	mPerChannelParams.reserve(MAX_CHANNELS);
	for (uint8_t ch = 0; ch < MAX_CHANNELS; ++ch) {
		mPerChannelParams.emplace_back(sampleFreq, ch);
	}

	reset(defaultSystemType);
}
ToneGenerator::~ToneGenerator()
{
}
// ---

// 各種パラメータ類 リセット
void ToneGenerator::reset(SystemType type)
{
	mSystemType = type;

	for (auto& params : mPerChannelParams) {
		params.reset(type);
	}
}

ToneGenerator::PerChannelParams::PerChannelParams(uint32_t sampleFreq, uint8_t ch)
	: sampleFreq(sampleFreq)
	, ch(ch)
{
}
// チャネル毎パラメータ類 リセット
void ToneGenerator::PerChannelParams::reset(SystemType type)
{
	_toneMapper.reset();
	_tones.clear();

	ccPan = 0.0f;
	ccExpression = 1.0;

	ccPrevCtrlNo = 0xFF; // invalid value
	ccPrevValue = 0x00;

	rpnNull = false;
	rpnPitchBendSensitibity = 2;

	resetParameterNumberState();

	pcId = 0; // Acoustic Piano
	updateProgram();
}

void ToneGenerator::PerChannelParams::resetParameterNumberState()
{
	ccRPN_MSB.reset();
	ccRPN_LSB.reset();
	ccNRPN_MSB.reset();
	ccNRPN_LSB.reset();
	ccDE_MSB.reset();
	ccDE_LSB.reset();
}
void ToneGenerator::PerChannelParams::noteOn(uint32_t noteNo, uint8_t vel)
{
	auto kvp = _toneMapper.noteOn(noteNo);
	tone_noteOff(kvp.second);
	tone_noteOn(kvp.first, noteNo, vel);
}
void ToneGenerator::PerChannelParams::noteOff(uint32_t noteNo)
{
	auto releasedTone = _toneMapper.noteOff(noteNo);
	tone_noteOff(releasedTone);
}
void ToneGenerator::PerChannelParams::holdOn()
{
	_toneMapper.holdOn();
}
void ToneGenerator::PerChannelParams::holdOff()
{
	auto releasedTones = _toneMapper.holdOff();
	for (auto& toneId : releasedTones) {
		tone_noteOff(toneId);
	}
}
std::pair<float,float> ToneGenerator::PerChannelParams::update()
{
	// オシレータからの出力はモノラル
	float v = 0;
	for (auto& kvp : _tones) {
		v += kvp.second->update();
	}
	
	// ステレオ化
	float lch = v * ccPan;
	float rch = v * (1.0f-ccPan);

	return {lch, rch};
}
void ToneGenerator::PerChannelParams::updateProgram()
{
	static const LSP::Filter::EnvelopeGenerator<float>::Curve curveExp3(3.0f);
	switch (pcId) {
	case 0:	// Acoustic Piano
	default:
		pcEG.setParam((float)sampleFreq, curveExp3, 0.1f, 0.0f, 0.2f, 0.25f, -10.0f, 0.05f);
		break;
	}
}
void ToneGenerator::PerChannelParams::tone_noteOn(ToneId id, uint32_t noteNo, uint8_t vel)
{
	float toneVolume = (vel / 127.0f);
	float freq = 440*log2((noteNo-69)/12.0f);

	SimpleTone::FunctionGenerator fg;
	fg.setSinWave(sampleFreq, freq);
	auto tone = std::make_unique<SimpleTone>(fg, pcEG, toneVolume);
	_tones.emplace(id, std::move(tone));
}
void ToneGenerator::PerChannelParams::tone_noteOff(ToneId id)
{
	auto found = _tones.find(id);
	if(found == _tones.end()) return;
	Tone& tone = *found->second;

	tone.envolopeGenerator().noteOff();
}

// ---


LSP::Signal<float> ToneGenerator::generate(size_t len)
{
	std::lock_guard lock(mMutex);

	auto sig = LSP::Signal<float>::allocate(&mMem, 2, len);
	for (size_t i = 0; i < len; ++i) {
		float lch = 0, rch=0;
		for (auto& params : mPerChannelParams) {
			auto v = params.update();
			lch += v.first;
			rch += v.second;
		}
		auto frame = sig.frame(i);
		frame[0] = lch;
		frame[1] = rch;
	}
	return sig;
}

// MIDIメッセージ受信コールバック
void ToneGenerator::onMidiMessageReceived(clock::time_point msg_time, const std::shared_ptr<const MIDI::Message>& msg){
	std::lock_guard lock(mMutex);
	mMessageQueue.emplace_back(msg_time, msg);
}
// 指定時刻時点までに蓄積されたMIDIメッセージを解釈します
void ToneGenerator::play(clock::time_point until)
{
	std::lock_guard lock(mMutex);
	while (!mMessageQueue.empty()) {
		const auto& [msg_time, msg] = mMessageQueue.front();
		if(msg_time >= until) break;

		dispatchMessage(msg);

		mMessageQueue.pop_front();
	}
}
void ToneGenerator::dispatchMessage(const std::shared_ptr<const MIDI::Message>& msg)
{
	// TODO 
}
// ---
// ノートオン
void ToneGenerator::noteOn(clock::time_point played_time, uint8_t ch, uint8_t noteNo, uint8_t vel)
{
	std::lock_guard lock(mMutex);
	if(ch >= mPerChannelParams.size()) return;
	auto& params = mPerChannelParams[ch];

	params.noteOn(noteNo, vel);
}

// ノートオフ
void ToneGenerator::noteOff(clock::time_point played_time, uint8_t ch, uint8_t noteNo, uint8_t vel)
{
	// MEMO 一般に、MIDIではノートオフの代わりにvel=0のノートオンが使用されるため、呼ばれることは希である

	std::lock_guard lock(mMutex);
	if(ch >= mPerChannelParams.size()) return;
	auto& params = mPerChannelParams[ch];

	params.noteOff(noteNo);
}

// コントロールチェンジ
void ToneGenerator::controlChange(clock::time_point played_time, uint8_t ch, uint8_t ctrlNo, uint8_t value)
{
	// 参考 : http://quelque.sakura.ne.jp/midi_cc.html

	std::lock_guard lock(mMutex);

	// ---
	
	if(ch >= mPerChannelParams.size()) return;
	auto& params = mPerChannelParams[ch];
	
	bool apply_RPN_NRPN_state = false;

	switch (ctrlNo) {
	case 6: // Data Entry(MSB)
		params.ccDE_MSB = value;
		params.ccDE_LSB.reset();
		apply_RPN_NRPN_state = true; // MSBのみでよいものはこのタイミングで適用する
		break;
	case 10: // Pan(パン)
		params.ccPan = (value / 127.0f);
		break;
	case 11: // Expression(エクスプレッション)
		params.ccExpression = (value / 127.0f);
		break;
	case 36: // Data Entry(LSB)
		params.ccDE_LSB = value;
		apply_RPN_NRPN_state = true; // MSBのみでよいものはこのタイミングで適用する
		break;
	case 64: // Hold1(ホールド1:ダンパーペダル)
		if(value < 0x64) {
			params.holdOff();
		} else {
			params.holdOn();
		}
		break;
	case 98: // NRPN(LSB)
		params.ccNRPN_LSB = value;
		break;
	case 99: // NRPN(MSB)
		params.resetParameterNumberState();
		params.ccNRPN_MSB = value;
		break;
	case 100: // RPN(LSB)
		params.ccRPN_LSB = value;
		break;
	case 101: // RPN(MSB)
		params.resetParameterNumberState();
		params.ccRPN_MSB = value;
		break;
	}

	// RPN/NRPNの受付が禁止されている場合、適用しない
	if (params.rpnNull) {
		apply_RPN_NRPN_state = false;
	}

	// RPN/NRPN 適用
	if (apply_RPN_NRPN_state) {
		if (params.ccRPN_MSB == 0 && params.ccRPN_MSB == 0 && params.ccDE_MSB.has_value()) {
			// ピッチベンドセンシティビティ: MSBのみ使用
			params.rpnPitchBendSensitibity = params.ccDE_MSB.value();
		}
	}

	params.ccPrevCtrlNo = ctrlNo;
	params.ccPrevValue = value;
}

// システムエクスクルーシブ
void ToneGenerator::sysExMessage(clock::time_point played_time, const uint8_t* data, size_t len)
{
	std::lock_guard lock(mMutex);

	// ---

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

		if (match({0x7F, 0x09, 0x01, 0xF7})) {
			// GM1 System On
			reset(SystemType::GM1);
		} 
		if(match({0x7F, 0x09, 0x03, 0xF7})) {
			// GM2 System On
			reset(SystemType::GM2);
		}
		if (match({0x7F, 0x09, 0x02, 0xF7})) {
			// GM System Off → GS Reset
			reset(SystemType::GS);
		}
	} 
	if (makerId = 0x41) {
		// Roland
		if (match({{/*dev:any*/}, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7})) {
			// GS Reset
			reset(SystemType::GS);
		}
	}
}
