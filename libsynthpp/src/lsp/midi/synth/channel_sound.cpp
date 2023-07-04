/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/midi/synth/channel_sound.hpp>
#include <lsp/midi/synth/instruments.hpp>
#include <lsp/midi/synth/voice.hpp>

using namespace lsp::midi::synth;

ChannelSound::ChannelSound(LuathSynth& synth, int ch)
	: mSynth(synth)
	, mMidiCh(ch)
{
	reset(midi::SystemType::GM1());
}
// チャネル毎パラメータ類 リセット
void ChannelSound::reset(midi::SystemType type)
{
	mSystemType = type;

	allNotesOff(false);
	resetParameters();
}
void ChannelSound::resetParameters()
{
	// チャネル ボイス メッセージ系
	mProgId = 0; // Acoustic Piano
	mIsDrumPart = (mMidiCh == 10);
	mRawPitchBend = 0;
	mCalculatedPitchBend = 0;

	// コントロールチェンジ系
	ccVolume = 1.0;
	ccPan = 0.5f;
	ccExpression = 1.0;
	ccPedal = false;

	ccPrevCtrlNo = 0xFF; // invalid value
	ccPrevValue = 0x00;

	ccBankSelectLSB = 0;
	ccBankSelectMSB = 0;

	ccAttackTime = 64;
	ccDecayTime = 64;
	ccReleaseTime = 64;

	mRPNDetector.reset();
	mRawRPNs.clear();
	mRawNRPNs.clear();
}

void ChannelSound::noteOn(int noteNo, float vel)
{
	// 同じノート番号は同時発音不可
	noteOff(noteNo);
	// ---
	if (vel > 0) {
		auto id = VoiceId::issue();
		auto voice = createVoice(noteNo, vel);
		mVoices.emplace(id, std::move(voice));
	}
}
void ChannelSound::noteOff(int noteNo, bool allowTailOff)
{
	for (auto& [id, voice] : mVoices) {
		if (voice->noteNo() == noteNo) {
			voice->noteOff(allowTailOff);
		}
	}
}
void ChannelSound::allNotesOff(bool allowTailOff)
{
	for(auto& [id, voice] : mVoices) {
		voice->noteOff(allowTailOff);
	}
	if(!allowTailOff) mVoices.clear();
}
// プログラムチェンジ
void ChannelSound::programChange(int progId)
{
	// 事前に受信していたバンクセレクトを解決
	// プログラムId更新
	mProgId = progId;
}
// コントロールチェンジ & チャネルモードメッセージ
void ChannelSound::controlChange(int ctrlNo, int value)
{
	// 参考 : http://quelque.sakura.ne.jp/midi_cc.html
	//        https://www.g200kg.com/jp/docs/tech/midi.html

	switch (ctrlNo) {
	// --- コントロールチェンジ ---
	case 0: // Bank Select <MSB>（バンクセレクト）
		ccBankSelectMSB = value;
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
	case 64: // Hold1(ホールド1:ダンパーペダル)
		ccPedal = (value >= 0x64);
		updateHold();
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
	// --- チャネルモードメッセージ ---
	case 121: // リセットオールコントローラ
		resetParameters();
		break;
	// --- チャネルモードメッセージ : not implemented ---
	case 122: // ローカルコントロール
	case 124: // オムニオフ
	case 125: // オムニオン
	case 126: // モノモード
	case 127: // モノモード
		break;
	}

	// RPN/NRPN 適用
	juce::MidiRPNMessage rpn;
	if (mRPNDetector.parseControllerMessage(mMidiCh, ctrlNo, value, rpn)) {
		if(rpn.isNRPN) {
			mRawNRPNs.insert_or_assign(rpn.parameterNumber, std::make_pair(rpn.value, rpn.is14BitValue));
		}else {
			mRawRPNs.insert_or_assign(rpn.parameterNumber, std::make_pair(rpn.value, rpn.is14BitValue));
		}
		const bool isRPN = !rpn.isNRPN;
		const uint8_t pnMSB = (0x3F80 & rpn.parameterNumber) >> 7;
		const uint8_t pnLSB = (0x007F & rpn.parameterNumber) >> 0;

		// RPN/ NRPN 即時反映系
		// - RPN: ピッチベンドセンシティビティ
		if(isRPN && pnMSB == 0 && pnLSB == 0) {
			updatePitchBend();
		}

		// - NRPN(XG) : ドラムパートへ切替
		if(mSystemType.isXG() && pnMSB == 127) {
			setDrumMode(true);
		}
	}

	ccPrevCtrlNo = ctrlNo;
	ccPrevValue = value;
}

void ChannelSound::pitchBend(int value)
{
	mRawPitchBend = static_cast<uint16_t>(value - 0x2000); // 0 to 0x3fff
	updatePitchBend();
}

StereoFrame ChannelSound::update()
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
		if (iter->second->envolopeGenerator().isBusy()) {
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

	return ret;
}
ChannelSound::Digest ChannelSound::digest()const
{
	Digest digest;

	digest.ch = mMidiCh;
	digest.progId = mProgId;
	digest.bankSelectMSB = ccBankSelectMSB;
	digest.bankSelectLSB = ccBankSelectLSB;
	digest.volume = ccVolume; 
	digest.expression = ccExpression;
	digest.pan = ccPan;
	digest.pitchBend = mCalculatedPitchBend;
	digest.attackTime = ccAttackTime;
	digest.decayTime = ccDecayTime;
	digest.releaseTime = ccReleaseTime;
	digest.poly = mVoices.size();
	digest.pedal = ccPedal;
	digest.drum = mIsDrumPart;

	for (auto& [id, voice] : mVoices) {
		digest.voices.emplace(id, voice->digest());
	}

	return digest;
}
std::unique_ptr<Voice> ChannelSound::createVoice(int noteNo, float vel)
{
	if(mIsDrumPart) {
		return createDrumVoice(noteNo, vel);
	}
	else {
		return createMelodyVoice(noteNo, vel);
	}
}
std::optional<int> ChannelSound::getInt14RPN(int msb, int lsb)const noexcept
{
	auto pn = ((msb & 0x7F) << 7) + (lsb & 0x7F);
	auto found = mRawRPNs.find(pn);
	if(found == mRawRPNs.end()) return {};
	auto [value, is14bit] = found->second;
	return (is14bit ? value : ((value & 0x7F) << 7)) - 0x2000;
}
std::optional<int> ChannelSound::getInt7RPN(int msb, int lsb)const noexcept
{
	auto found = getInt14RPN(msb, lsb);
	return found ? std::make_optional(found.value() / 0x80) : std::nullopt;
}
std::optional<int> ChannelSound::getInt14NRPN(int msb, int lsb)const noexcept
{
	auto pn = ((msb & 0x7F) << 7) + (lsb & 0x7F);
	auto found = mRawNRPNs.find(pn);
	if(found == mRawNRPNs.end()) return {};
	auto [value, is14bit] = found->second;
	return (is14bit ? value : ((value & 0x7F) << 7)) - 0x2000;
}
std::optional<int> ChannelSound::getInt7NRPN(int msb, int lsb)const noexcept
{
	auto found = getInt14NRPN(msb, lsb);
	return found ? std::make_optional(found.value() / 0x80) : std::nullopt;
}
void ChannelSound::updatePitchBend()
{
	auto pitchBendSensitivity = getInt7NRPN(0, 0).value_or(mSystemType.isOnlyGM1() ? 12 : 2);
	auto masterCourseTuning = getInt7NRPN(0, 2).value_or(0);
	auto masterFineTuning = getInt14NRPN(0, 1).value_or(0) / 8192.f;
	mCalculatedPitchBend 
		= pitchBendSensitivity * (mRawPitchBend / 8192.0f)
		+ masterCourseTuning
		+ masterFineTuning;

	for (auto& kvp : mVoices) {
		kvp.second->setPitchBend(mCalculatedPitchBend);
	}
}
void ChannelSound::updateHold()
{
	for (auto& [id, voice] : mVoices) {
		voice->setHold(ccPedal);
	}
}
void ChannelSound::setDrumMode(bool isDrum)
{
	if(mIsDrumPart != isDrum) {
		mVoices.clear();
		mIsDrumPart = isDrum;
	}
}