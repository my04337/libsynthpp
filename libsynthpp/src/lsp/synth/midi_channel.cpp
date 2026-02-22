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
	ccResonance = 64;
	ccBrightness = 64;

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
	case 71: // Resonance(レゾナンス / ハーモニックコンテント)
		ccResonance = value;
		updateFilter();
		break;
	case 72: // Release Time(リリースタイム)
		ccReleaseTime = value;
		updateReleaseTime();
		break;
	case 73: // Attack Time(アタックタイム)
		ccAttackTime = value;
		break;
	case 74: // Brightness(ブライトネス / カットオフ)
		ccBrightness = value;
		updateFilter();
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

			// - NRPN(GS/XG) : EGリリースタイム
			if((mSystemType.isGS() || mSystemType.isXG()) && ccNRPN_MSB == 1 && ccNRPN_LSB == 102) {
				updateReleaseTime();
			}

			// - NRPN(GS/XG) : カットオフ / レゾナンス
			if((mSystemType.isGS() || mSystemType.isXG()) && ccNRPN_MSB == 1 && (ccNRPN_LSB == 32 || ccNRPN_LSB == 33)) {
				updateFilter();
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
		if (iter->second->isBusy()) {
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
	digest.brightness = ccBrightness;
	digest.resonance = ccResonance;
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
float MidiChannel::calcEGTimeScale(uint8_t ccValue)
{
	// CC値(0-127, 中心值64)を対数スケーリング係数に変換します
	// 中心值64で等倍(1.0)、最小値で0で約x0.006、最大值127で約x190
	return powf(10.0f, (ccValue / 128.f - 0.5f) * 4.556f);
}
float MidiChannel::calcReleaseTimeScale()const
{
	float scale = calcEGTimeScale(ccReleaseTime);
	// NRPN (1, 102) : GS/XGのパート別リリースタイムオフセット
	if (mSystemType.isGS() || mSystemType.isXG()) {
		scale *= calcEGTimeScale(getNRPN_MSB(1, 102).value_or(64));
	}
	return scale;
}
void MidiChannel::updateReleaseTime()
{
	float scale = calcReleaseTimeScale();
	for (auto& [id, voice] : mVoices) {
		voice->setReleaseTimeScale(scale);
	}
}
float MidiChannel::calcFilterCutoff(float noteFreq)const
{
	// CC#74 (Brightness) → ローパスフィルタのカットオフ周波数
	// CC値を対数スケーリングでカットオフ周波数に変換します
	// CC 0 → noteFreq × 1 (基本周波数 = 暗い音)
	// CC 64 → noteFreq × 16 (デフォルト、ほぼフルオープン)
	// CC 127 → noteFreq × 256 (完全にフルオープン)
	float ccRate = exp2f(ccBrightness / 16.f); // 2^0=1 ~ 2^(127/16)≈2^8=256

	// NRPN (1, 32) : GS/XGのパート別カットオフオフセット (中心值64でオフセットなし)
	// ±4オクターブの範囲で調整
	float nrpnScale = 1.f;
	if(mSystemType.isGS() || mSystemType.isXG()) {
		int nrpnOffset = static_cast<int>(getNRPN_MSB(1, 32).value_or(64)) - 64;
		nrpnScale = exp2f(nrpnOffset / 16.f); // ±4オクターブ
	}

	float cutoff = noteFreq * ccRate * nrpnScale;

	// ナイキスト周波数以下に制限
	float nyquist = mSampleFreq / 2.f;
	return std::clamp(cutoff, 20.f, nyquist * 0.95f);
}
float MidiChannel::calcFilterQ()const
{
	// CC#71 (Resonance) → ローパスフィルタのQ値
	// CC 0 → Q = 0.707 (バターワース、共振なし)
	// CC 64 → Q = 0.707 (デフォルト、フラット)
	// CC 127 → Q ≈ 12 (強い共振ピーク)
	float baseQ = 0.707f;
	if(ccResonance > 64) {
		// 64超で共振が増加 : 65→微小な共振、127→強い共振
		float t = (ccResonance - 64) / 63.f; // 0.0 ~ 1.0
		baseQ = 0.707f + t * t * 11.3f; // 0.707 ~ 12.0 (二次曲線で急峻に上昇)
	}

	// NRPN (1, 33) : GS/XGのパート別レゾナンスオフセット (中心值64でオフセットなし)
	if(mSystemType.isGS() || mSystemType.isXG()) {
		int nrpnValue = static_cast<int>(getNRPN_MSB(1, 33).value_or(64));
		if(nrpnValue > 64) {
			float t = (nrpnValue - 64) / 63.f;
			baseQ += t * t * 5.f; // NRPNで追加の共振
		}
	}

	return std::clamp(baseQ, 0.5f, 20.f);
}
void MidiChannel::updateFilter()
{
	float Q = calcFilterQ();
	for (auto& [id, voice] : mVoices) {
		float noteFreq = 440.f * exp2f((voice->soundingNoteNo() - 69.f) / 12.f);
		float cutoff = calcFilterCutoff(noteFreq);
		voice->setFilter(cutoff, Q);
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