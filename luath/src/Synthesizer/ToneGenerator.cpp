#include <Luath/Syntesizer/ToneGenerator.hpp>

using namespace LSP;
using namespace Luath::Synthesizer;

LuathToneGenerator::LuathToneGenerator(SystemType defaultSystemType)
{
	reset(defaultSystemType);

	uint8_t ch = 0;
	for (auto& params : mPerChannelParams) {
		params.ch = ch++;
		params.toneMapper.setCallback(
			[&params](ToneId toneNo, uint32_t noteNo, uint8_t vel) 
		{ 
			params.onToneStateChanged(toneNo, noteNo, vel);
		});
	}
}
LuathToneGenerator::~LuathToneGenerator()
{
	for (auto& params : mPerChannelParams) {
		params.toneMapper.setCallback(nullptr);
	}
}
// 各種パラメータ類 リセット
void LuathToneGenerator::reset(SystemType type)
{
	mSystemType = type;

	for (auto& params : mPerChannelParams) {
		params.reset(type);
	}
}
// チャネル毎パラメータ類 リセット
void LuathToneGenerator::PerChannelParams::reset(SystemType type)
{
	toneMapper.reset();

	ccPan = 0.0f;
	ccExpression = 1.0;

	ccPrevCtrlNo = 0xFF; // invalid value
	ccPrevValue = 0x00;

	rpnNull = false;
	rpnPitchBendSensitibity = 2;

	resetPNState();
}

void LuathToneGenerator::PerChannelParams::resetPNState()
{
	ccRPN_MSB.reset();
	ccRPN_LSB.reset();
	ccNRPN_MSB.reset();
	ccNRPN_LSB.reset();
	ccDE_MSB.reset();
	ccDE_LSB.reset();
}
void LuathToneGenerator::PerChannelParams::onToneStateChanged(ToneId toneNo, uint32_t noteNo, uint8_t vel)
{
	if(vel > 0) {
		LSP::Log::d(LOGF("NoteOn  : "<< (int)ch << " - " << (int)noteNo << " (id:" << toneNo.id() << ")"));
	} else {
		LSP::Log::d(LOGF("NoteOff : "<< (int)ch << " - " << (int)noteNo << " (id:" << toneNo.id() << ")"));
	}

}

// ---

// ノートオン
void LuathToneGenerator::noteOn(uint8_t ch, uint8_t noteNo, uint8_t vel)
{
	std::lock_guard lock(mMutex);
	if(ch >= mPerChannelParams.size()) return;
	auto& params = mPerChannelParams[ch];

	if(vel > 0) {
		// ノートオン
		auto ToneId = params.toneMapper.noteOn(noteNo, vel);
	} else {
		// vel=0:ノートオフと見なす(MIDIの慣習)
		params.toneMapper.noteOff(noteNo);
	}

}

// ノートオフ
void LuathToneGenerator::noteOff(uint8_t ch, uint8_t noteNo, uint8_t vel)
{
	// MEMO 一般に、MIDIではノートオフの代わりにvel=0のノートオンが使用されるため、呼ばれることは希である

	std::lock_guard lock(mMutex);
	if(ch >= mPerChannelParams.size()) return;
	auto& params = mPerChannelParams[ch];

	params.toneMapper.noteOff(noteNo);
}

// コントロールチェンジ
void LuathToneGenerator::controlChange(uint8_t ch, uint8_t ctrlNo, uint8_t value)
{
	// 参考 : http://quelque.sakura.ne.jp/midi_cc.html

	std::lock_guard lock(mMutex);

	// ---
	
	if(ch >= mPerChannelParams.size()) return;
	auto& params = mPerChannelParams[ch];
	
	bool hold_RPN_NRPN_state = false;
	bool apply_RPN_NRPN_state = false;

	switch (ctrlNo) {
	case 6: // Data Entry(MSB)
		params.ccDE_MSB = value;
		hold_RPN_NRPN_state = true;
		apply_RPN_NRPN_state = true; // MSBのみでよいものはこのタイミングで適用する
		break;
	case 10: // Pan(パン)
		params.ccPan = (value / 127.0f)-0.5f;
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
			params.toneMapper.holdOff();
		} else {
			params.toneMapper.holdOn();
		}
		break;
	case 98: // NRPN(LSB)
		params.ccNRPN_LSB = value;
		hold_RPN_NRPN_state = false;
		break;
	case 99: // NRPN(MSB)
		params.resetPNState();
		params.ccNRPN_MSB = value;
		break;
	case 100: // RPN(LSB)
		params.ccRPN_LSB = value;
		hold_RPN_NRPN_state = false;
		break;
	case 101: // RPN(MSB)
		params.resetPNState();
		params.ccRPN_MSB = value;
		hold_RPN_NRPN_state = false;
		break;
	}

	// RPN/NRPNの受付が禁止されている場合、適用しない
	if (params.rpnNull) {
		apply_RPN_NRPN_state = false;
		hold_RPN_NRPN_state = false;
	}

	// RPN/NRPN 適用
	if (apply_RPN_NRPN_state) {
		if (params.ccRPN_MSB == 0 && params.ccRPN_MSB == 0 && params.ccDE_MSB.has_value()) {
			// ピッチベンドセンシティビティ: MSBのみ使用
			params.rpnPitchBendSensitibity = params.ccDE_MSB.value();
			hold_RPN_NRPN_state = false;
		}
	}

	// RPN/NRPN リセット
	if (!hold_RPN_NRPN_state) {
		params.resetPNState();
	}

	params.ccPrevCtrlNo = ctrlNo;
	params.ccPrevValue = value;
}

// システムエクスクルーシブ
void LuathToneGenerator::sysExMessage(const uint8_t* data, size_t len)
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
