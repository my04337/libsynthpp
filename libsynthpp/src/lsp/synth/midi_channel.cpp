#include <lsp/synth/midi_channel.hpp>
#include <lsp/synth/voice.hpp>

using namespace lsp::synth;

MidiChannel::MidiChannel(uint32_t sampleFreq, uint8_t ch)
	: mSampleFreq(sampleFreq)
	, mMidiCh(ch)
{
	reset(midi::SystemType::GM1());
}
// チャネル毎パラメータ類 リセット
void MidiChannel::reset(midi::SystemType type)
{
	mSystemType = type;

	resetVoices();
	resetParameters();
}
void MidiChannel::resetVoices()
{
	// 全発音を強制停止
	mVoices.clear();
}
void MidiChannel::resetParameters()
{
	// チャネル ボイス メッセージ系
	mProgId = 0; // Acoustic Piano
	mIsDrumPart = (mMidiCh == 9);
	mRawPitchBend = 0;
	mCalculatedPitchBend = 0;
	mChannelPressure = 1.0f;

	// コントロールチェンジ系
	ccVolume = 1.0;
	ccPan = 0.5f;
	ccExpression = 1.0;
	ccPedal = false;
	ccSostenuto = false;

	ccPrevCtrlNo = 0xFF; // invalid value
	ccPrevValue = 0x00;

	ccBankSelectLSB = 0;
	ccBankSelectMSB = 0;

	ccAttackTime = 64;
	ccDecayTime = 64;
	ccReleaseTime = 64;

	mMonoMode = false;

	resetParameterNumberState();

	ccRPNs.clear();
	ccNRPNs.clear();
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
	// 同じノート番号は同時発音不可
	noteOff(noteNo);
	// ---
	if (vel > 0) {
		// モノモード時は既存の全ボイスを停止してから新しいボイスを発音
		if (mMonoMode) {
			for (auto& [id, voice] : mVoices) {
				voice->noteOff();
			}
		}
		auto id = VoiceId::issue();
		auto voice = createVoice(noteNo, vel);
		mVoices.emplace(id, std::move(voice));
	}
}
void MidiChannel::noteOff(uint32_t noteNo)
{
	for (auto& [id, voice] : mVoices) {
		if (voice->noteNo() == noteNo) {
			voice->noteOff();
		}
	}
}
void MidiChannel::noteCut(uint32_t noteNo)
{
	for(auto& [id, voice] : mVoices) {
		if(voice->noteNo() == noteNo) {
			voice->noteCut();
		}
	}
}
// プログラムチェンジ
void MidiChannel::programChange(uint8_t progId)
{
	// 事前に受信していたバンクセレクトを解決
	// プログラムId更新
	mProgId = progId;
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
		ccPedal = (value >= 0x40);
		updateHold();
		break;
	case 66: // Sostenuto(ソステヌート)
		ccSostenuto = (value >= 0x40);
		updateSostenuto();
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
		apply_RPN_NRPN_state = true; // RPNヌルはこのタイミングで適用する
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
		for (auto& kvp : mVoices) {
			kvp.second->noteCut();
		}
		break;
	// --- チャネルモードメッセージ : not implemented ---
	case 122: // ローカルコントロール
	case 124: // オムニオフ
	case 125: // オムニオン
		break;
	case 126: // モノモード
		mMonoMode = true;
		for (auto& kvp : mVoices) {
			kvp.second->noteOff();
		}
		break;
	case 127: // ポリモード
		mMonoMode = false;
		for (auto& kvp : mVoices) {
			kvp.second->noteOff();
		}
		break;
	}

	// RPN/NRPN 適用
	if (apply_RPN_NRPN_state) {
		if(ccRPN_MSB == 0x7F && ccRPN_LSB == 0x7F) {
			// RPNヌル
			resetParameterNumberState();
		}
		else if(ccDE_MSB || ccDE_LSB) {
			if(ccRPN_MSB && ccRPN_LSB) {
				auto key = (static_cast<uint16_t>(*ccRPN_MSB) << 8) | *ccRPN_LSB;
				if(ccDE_LSB) {
					ccRPNs[key].second = *ccDE_LSB;
				}
				else {
					ccRPNs[key].first = *ccDE_MSB;
				}
			}
			if(ccNRPN_MSB && ccNRPN_LSB) {
				auto key = (static_cast<uint16_t>(*ccNRPN_MSB) << 8) | *ccNRPN_LSB;
				if(ccDE_LSB) {
					ccNRPNs[key].second = *ccDE_LSB;
				}
				else {
					ccNRPNs[key].first = *ccDE_MSB;
				}
			}

			// RPN/ NRPN 即時反映系
			// - RPN: ピッチベンドセンシティビティ
			if(ccRPN_MSB == 0 && ccRPN_LSB == 0 && ccDE_MSB) {
				updatePitchBend();
			}

			// - NRPN(XG) : ドラムパートへ切替
			if(mSystemType.isXG() && ccNRPN_MSB == 127) {
				setDrumMode(true);
			}
		}
	}

	ccPrevCtrlNo = ctrlNo;
	ccPrevValue = value;
}

void MidiChannel::pitchBend(int16_t pitch)
{
	mRawPitchBend = pitch;
	updatePitchBend();
}
void MidiChannel::channelPressure(uint8_t value)
{
	mChannelPressure = value / 127.0f;
}
void MidiChannel::polyphonicKeyPressure(uint8_t noteNo, uint8_t value)
{
	float pressure = value / 127.0f;
	for (auto& [id, voice] : mVoices) {
		if (static_cast<uint8_t>(voice->noteNo()) == noteNo) {
			voice->setPolyPressure(pressure);
		}
	}
}

StereoFrame MidiChannel::update()
{
	// オシレータからの出力はモノラル
	StereoFrame ret = std::make_pair(0.0f, 0.0f);
	for (auto iter = mVoices.begin(); iter != mVoices.end();) {
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
				pan = 1.0f - (1.0f - pan) * ((1.0f - vpan) * 2);
			}
		}

		ret.first += v * (1.0f - pan); // L ch
		ret.second += v * pan;         // R ch


		// 発音終了済のボイスを破棄
		if (iter->second->envelopeGenerator().isBusy()) {
			++iter;
		} else {
			iter = mVoices.erase(iter);
		}
	}

	// ボリューム
	ret.first *= ccVolume;
	ret.second *= ccVolume;

	// エクスプレッション
	ret.first *= ccExpression;
	ret.second *= ccExpression;

	// チャネルプレッシャーは現状未適用
	// 対応するインストゥルメントが存在しないため、Voice出力への反映は保留とする

	return ret;
}
MidiChannel::Digest MidiChannel::digest()const
{
	Digest digest;

	digest.ch = mMidiCh;
	digest.progId = mProgId;
	digest.bankSelectMSB = ccBankSelectMSB;
	digest.bankSelectLSB = ccBankSelectLSB;
	digest.volume = ccVolume; 
	digest.expression = ccExpression;
	digest.channelPressure = mChannelPressure;
	digest.pan = ccPan;
	digest.pitchBend = mCalculatedPitchBend;
	digest.attackTime = ccAttackTime;
	digest.decayTime = ccDecayTime;
	digest.releaseTime = ccReleaseTime;
	digest.poly = mVoices.size();
	digest.pedal = ccPedal;
	digest.sostenuto = ccSostenuto;
	digest.mono = mMonoMode;
	digest.drum = mIsDrumPart;

	for (auto& [id, voice] : mVoices) {
		digest.voices.emplace(id, voice->digest());
	}

	return digest;
}
std::unique_ptr<Voice> MidiChannel::createVoice(uint8_t noteNo, uint8_t vel)
{
	if(mIsDrumPart) {
		return createDrumVoice(noteNo, vel);
	}
	else {
		return createMelodyVoice(noteNo, vel);
	}
}
std::optional<uint8_t> MidiChannel::getRPN_MSB(uint8_t msb, uint8_t lsb)const noexcept
{
	auto found = ccRPNs.find((static_cast<uint16_t>(msb) << 8) | lsb);
	return found != ccRPNs.end() ? std::make_optional<uint8_t>(found->second.first) : std::nullopt;
}
std::optional<uint8_t> MidiChannel::getRPN_LSB(uint8_t msb, uint8_t lsb)const noexcept
{
	auto found = ccRPNs.find((static_cast<uint16_t>(msb) << 8) | lsb);
	return found != ccRPNs.end() ? found->second.second : std::nullopt;
}
std::optional<uint8_t> MidiChannel::getNRPN_MSB(uint8_t msb, uint8_t lsb)const noexcept
{
	auto found = ccNRPNs.find((static_cast<uint16_t>(msb) << 8) | lsb);
	return found != ccNRPNs.end() ? std::make_optional<uint8_t>(found->second.first) : std::nullopt;
}
std::optional<uint8_t> MidiChannel::getNRPN_LSB(uint8_t msb, uint8_t lsb)const noexcept
{
	auto found = ccNRPNs.find((static_cast<uint16_t>(msb) << 8) | lsb);
	return found != ccNRPNs.end() ? found->second.second : std::nullopt;
}
void MidiChannel::updatePitchBend()
{
	auto pitchBendSensitivity = getRPN_MSB(0, 0).value_or(mSystemType.isOnlyGM1() ? 12 : 2);
	auto masterCoarseTuning = getRPN_MSB(0, 2).value_or(64) - 64;
	auto masterFineTuning = ((getRPN_MSB(0, 1).value_or(64) - 64) * 128 + (getRPN_LSB(0, 1).value_or(64) - 64)) / 8192.f;
	mCalculatedPitchBend 
		= pitchBendSensitivity * (mRawPitchBend / 8192.0f)
		+ masterCoarseTuning
		+ masterFineTuning;

	for (auto& kvp : mVoices) {
		kvp.second->setPitchBend(mCalculatedPitchBend);
	}
}
void MidiChannel::updateHold()
{
	for (auto& [id, voice] : mVoices) {
		voice->setHold(ccPedal);
	}
}
// ソステヌートペダル(CC:66)の状態変化時に呼ばれます
// ダンパーペダル(CC:64)との違い :
//   - ダンパーペダル : ペダルON中の全てのnoteOffを保留する(チャネル全体)
//   - ソステヌート   : ペダルを踏んだ瞬間に打鍵中のボイスのみを保持対象とする
//     ソステヌートON後に新たに打鍵された音は保持対象外となる
// ※ createVoice()でソステヌート状態を渡さないのはこの仕様による意図的な設計です
void MidiChannel::updateSostenuto()
{
	if (ccSostenuto) {
		// ソステヌートON: 現在キーが押下中のボイスのみをソステヌート対象にする
		// isNoteOn()はnoteOff未受信かつリリース/止音状態でないことを確認する
		for (auto& [id, voice] : mVoices) {
			if (voice->isNoteOn()) {
				voice->setSostenuto(true);
			}
		}
	} else {
		// ソステヌートOFF: 全ボイスのソステヌートを解除
		// Hold(CC:64)も無効であれば、保留中のnoteOffが実行される
		for (auto& [id, voice] : mVoices) {
			voice->setSostenuto(false);
		}
	}
}
void MidiChannel::setDrumMode(bool isDrum)
{
	if(mIsDrumPart != isDrum) {
		mVoices.clear();
		mIsDrumPart = isDrum;
	}
}