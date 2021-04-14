#include <LSP/Synth/MidiChannel.hpp>
#include <LSP/Synth/WaveTable.hpp>
#include <LSP/Synth/Voice.hpp>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::Synth;

MidiChannel::MidiChannel(uint32_t sampleFreq, uint8_t ch, const WaveTable& waveTable)
	: sampleFreq(sampleFreq)
	, ch(ch)
	, _waveTable(waveTable)
{
	reset(LSP::MIDI::SystemType::GM1);
}
// チャネル毎パラメータ類 リセット
void MidiChannel::reset(SystemType type)
{
	systemType = type;

	resetVoices();
	resetParameters();

	progId = 0; // Acoustic Piano
}
void MidiChannel::resetVoices()
{
	_voiceMapper.reset();
	_voices.clear();
}
void MidiChannel::resetParameters()
{
	cmPitchBend = 0;
	calculatedPitchBend = 0;

	ccVolume = 1.0;
	ccPan = 0.5f;
	ccExpression = 1.0;

	ccPrevCtrlNo = 0xFF; // invalid value
	ccPrevValue = 0x00;

	ccBankSelectLSB = 0;
	ccBankSelectMSB = 0;

	ccAttackTime = 64;
	ccDecayTime = 64;
	ccReleaseTime = 64;

	isDrumPart = (ch == 9);

	rpnNull = false;
	rpnPitchBendSensitibity = 2;

	resetParameterNumberState();
}

void MidiChannel::resetParameterNumberState()
{
	ccRPN_MSB.reset();
	ccRPN_LSB.reset();
	ccNRPN_MSB.reset();
	ccNRPN_LSB.reset();
	ccDE_MSB.reset();
	ccDE_LSB.reset();
}
void MidiChannel::noteOn(uint32_t noteNo, uint8_t vel)
{
	auto kvp = _voiceMapper.noteOn(noteNo);
	voice_noteOff(kvp.second);
	if (vel > 0) {
		voice_noteOn(kvp.first, noteNo, vel);
	}
}
void MidiChannel::noteOff(uint32_t noteNo)
{
	auto releasedTone = _voiceMapper.noteOff(noteNo);
	voice_noteOff(releasedTone);
}
// プログラムチェンジ
void MidiChannel::programChange(uint8_t progId)
{
	// 事前に受信していたバンクセレクトを解決
	// プログラムId更新
	this->progId = progId;
}
// コントロールチェンジ & チャネルモードメッセージ
void MidiChannel::controlChange(uint8_t ctrlNo, uint8_t value)
{
	// 参考 : http://quelque.sakura.ne.jp/midi_cc.html
	//        https://www.g200kg.com/jp/docs/tech/midi.html

	bool apply_RPN_NRPN_state = false;

	switch (ctrlNo) {
	// --- コントロールチェンジ ---
	case 0: // Bank Select <MSB>（バンクセレクト）
		ccBankSelectMSB = value;
		break;
	case 6: // Data Entry(MSB)
		ccDE_MSB = value;
		ccDE_LSB.reset();
		apply_RPN_NRPN_state = true; // MSBのみでよいものはこのタイミングで適用する
		break;
	case 10: // Pan(パン)
		// MEMO 中央値は64。 1-127の範囲を取る実装が多い
		ccPan = std::clamp((value - 1) / 126.0f, 0.0f, 1.0f);
		break;
	case 7: // Channel Volumeチャンネルボリューム）
		ccVolume = (value / 127.0f);
		break;
	case 11: // Expression(エクスプレッション)
		ccExpression = (value / 127.0f);
		break;
	case 32: // Bank Select <LSB>（バンクセレクト）
		ccBankSelectLSB = value;
		break;
	case 36: // Data Entry(LSB)
		ccDE_LSB = value;
		apply_RPN_NRPN_state = true; // MSBのみでよいものはこのタイミングで適用する
		break;
	case 64: // Hold1(ホールド1:ダンパーペダル)
		if (value < 0x64) {
			holdOff();
		} else {
			holdOn();
		}
		break;
	case 72: // Release Time(リリースタイム)
		ccReleaseTime = value;
		break;
	case 73: // Attack Time(アタックタイム)
		ccAttackTime = value;
		break;
	case 75: // Decay Time(ディケイタイム)
		ccDecayTime = value;
		break;
	case 98: // NRPN(LSB)
		ccNRPN_LSB = value;
		break;
	case 99: // NRPN(MSB)
		resetParameterNumberState();
		ccNRPN_MSB = value;
		break;
	case 100: // RPN(LSB)
		ccRPN_LSB = value;
		break;
	case 101: // RPN(MSB)
		resetParameterNumberState();
		ccRPN_MSB = value;
		break;

	// --- チャネルモードメッセージ ---
	case 120: // オールサウンドオフ
		resetVoices();
		break;
	case 121: // リセットオールコントローラ
		resetParameters();
		break;
	case 123: // オールノートオフ
		_voiceMapper.reset();
		for (auto& kvp : _voices) {
			kvp.second->envolopeGenerator().noteOff();
		}
		break;
	// --- チャネルモードメッセージ : not implemented ---
	case 122: // ローカルコントロール
	case 124: // オムニオフ
	case 125: // オムニオン
	case 126: // モノモード
	case 127: // モノモード
		break;
	}

	// RPN/NRPNの受付が禁止されている場合、適用しない
	if (rpnNull) {
		apply_RPN_NRPN_state = false;
	}

	// RPN/NRPN 適用
	if (apply_RPN_NRPN_state) {
		if (ccRPN_MSB == 0 && ccRPN_LSB == 0 && ccDE_MSB.has_value()) {
			// ピッチベンドセンシティビティ: MSBのみ使用
			rpnPitchBendSensitibity = ccDE_MSB.value();
			applyPitchBend();
		}
	}

	ccPrevCtrlNo = ctrlNo;
	ccPrevValue = value;
}

void MidiChannel::pitchBend(int16_t pitch)
{
	cmPitchBend = pitch;
	applyPitchBend();
}

void MidiChannel::holdOn()
{
	_voiceMapper.holdOn();
}
void MidiChannel::holdOff()
{
	auto releasedTones = _voiceMapper.holdOff();
	for (auto& toneId : releasedTones) {
		voice_noteOff(toneId);
	}
}
StereoFrame MidiChannel::update()
{
	// オシレータからの出力はモノラル
	StereoFrame ret = std::make_pair(0.0f, 0.0f);
	for (auto iter = _voices.begin(); iter != _voices.end();) {
		auto& voice = *iter->second;
		// ボイス単体の音を生成
		float v = voice.update();

		// パン適用
		float pan = ccPan;
		if (voice.pan().has_value()) {
			float vpan = *voice.pan();
			if (vpan < 0.5f) {
				// vpan=0 : 左, vpan=0.5 : 元のpan
				pan = pan * (vpan * 2);
			} else {
				// vpan=0.5 : 元のpan, vpan=1.0 : 右
				pan = 1.0f - (1.0f - pan) * ((1.0 - vpan) * 2);
			}
		}

		ret.first += v * (1.0f - pan); // L ch
		ret.second += v * pan;         // R ch


		// 発音終了済のボイスを破棄
		if (iter->second->envolopeGenerator().isBusy()) {
			++iter;
		} else {
			iter = _voices.erase(iter);
		}
	}

	// ボリューム
	ret.first *= ccVolume;
	ret.second *= ccVolume;

	// エクスプレッション
	ret.first *= ccExpression;
	ret.second *= ccExpression;

	return ret;
}
MidiChannel::Info MidiChannel::info()const
{
	Info info;

	info.ch = ch;
	info.progId = progId;
	info.bankSelectMSB = ccBankSelectMSB;
	info.bankSelectLSB = ccBankSelectLSB;
	info.volume = ccVolume; 
	info.expression = ccExpression;
	info.pan = ccPan;
	info.pitchBend = calculatedPitchBend;
	info.attackTime = ccAttackTime;
	info.decayTime = ccDecayTime;
	info.releaseTime = ccReleaseTime;
	info.poly = _voices.size();
	info.pedal = _voiceMapper.isHolding();
	info.drum = isDrumPart;

	for (auto& kvp : _voices) {
		info.voiceInfo.emplace(kvp.first, kvp.second->info());
	}

	return info;
}
std::unique_ptr<LSP::Synth::Voice> MidiChannel::createVoice(uint8_t noteNo, uint8_t vel)
{
	const float volume = (vel / 127.0f);
	static const LSP::Filter::EnvelopeGenerator<float>::Curve curveExp3(3.0f);
	Voice::EnvelopeGenerator eg;
	auto makeWaveTableVoice = [&](size_t waveTableId) {
		auto wg = _waveTable.get(waveTableId);
		return std::make_unique<LSP::Synth::WaveTableVoice>(sampleFreq, wg, eg, noteNo, calculatedPitchBend, volume);
	};

	if (!isDrumPart) {
		switch (progId) {
		case 0:	// Acoustic Piano
		default:
			eg.setParam((float)sampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.25f, -1.0f, 0.05f);
			return makeWaveTableVoice(WaveTable::Preset::SquareWave);
		}
	} else {
		// TODO ドラム用音色を用意する
		eg.setParam((float)sampleFreq, curveExp3, 0.05f, 0.0f, 0.2f, 0.25f, -1.0f, 0.05f);
		return makeWaveTableVoice(WaveTable::Preset::Ground);
	}
}
void MidiChannel::applyPitchBend()
{
	calculatedPitchBend = rpnPitchBendSensitibity * (cmPitchBend / 8192.0f);
	for (auto& kvp : _voices) {
		kvp.second->setPitchBend(calculatedPitchBend);
	}
}
void MidiChannel::voice_noteOn(VoiceId id, uint32_t noteNo, uint8_t vel)
{
	auto voice = createVoice(noteNo, vel);
	_voices.emplace(id, std::move(voice));
}
void MidiChannel::voice_noteOff(VoiceId id)
{
	auto found = _voices.find(id);
	if (found == _voices.end()) return;
	auto& voice = *found->second;

	voice.envolopeGenerator().noteOff();
}
