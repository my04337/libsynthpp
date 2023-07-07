/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#include <lsp/synth/state.hpp>
#include <lsp/synth/instruments.hpp>
#include <lsp/synth/voice.hpp>
#include <lsp/synth/synth.hpp>

using namespace lsp::synth;

namespace {

float uint7ToFloat(int value, float min, float max) {
	auto normalized = std::clamp((value - 1) / 126.0f, 0.0f, 1.0f);
	return min + normalized * (max - min);
};
float uint14ToFloat(int value, float min, float max) {
	auto normalized = std::clamp((value - 1) / 16382.f, 0.0f, 1.0f);
	return min + normalized * (max - min);
};

float int7ToFloat(int value, float min, float max) {
	return uint7ToFloat(value + 64, min, max);
};
float int14ToFloat(int value, float min, float max) {
	return uint14ToFloat(value + 8192, min, max);
};

}

ChannelState::ChannelState(LuathSynth& synth, int ch)
	: synth(synth)
	, midiChannel(ch)
{
	reset();
}
// チャネル毎パラメータ類 リセット
void ChannelState::reset()
{
	// チャネル ボイス メッセージ系
	progId = 0; // Acoustic Piano
	bankSelectMSB = 0;
	bankSelectLSB = 0;

	isDrumPart = (midiChannel == 10);
	pitchBendWithoutSensitivity = 0.f;

	// コントロールチェンジ系
	ccVolume = 1.0f;
	ccPanpot = 0.5f;
	ccExpression = 1.0;
	ccPedal = false;

	ccAttackTime = 0.5f;
	ccDecayTime = 0.5f;
	ccReleaseTime = 0.5f;

	mRPNDetector.reset();
	mRawRPNs.clear();
	mRawNRPNs.clear();
}

// コントロールチェンジ & チャネルモードメッセージ
void ChannelState::handleController(int ctrlNo, int value)
{
	// 参考 : http://quelque.sakura.ne.jp/midi_cc.html
	//        https://www.g200kg.com/jp/docs/tech/midi.html


	switch (ctrlNo) {
	// --- コントロールチェンジ ---
	case 0: // Bank Select <MSB>（バンクセレクト）
		bankSelectMSB = value;
		break;
	case 10: // Pan(パン)
		// MEMO 中央値は64。 1-127の範囲を取る実装が多い
		ccPanpot = uint7ToFloat(value, 0, 1);
		break;
	case 7: // Channel Volumeチャンネルボリューム）
		ccVolume = uint7ToFloat(value, 0, 1);
		break;
	case 11: // Expression(エクスプレッション)
		ccExpression = uint7ToFloat(value, 0, 1);
		break;
	case 32: // Bank Select <LSB>（バンクセレクト）
		bankSelectLSB = value;
		break;
	case 64: // Hold1(ホールド1:ダンパーペダル)
		ccPedal = (value >= 0x64);
		break;
	case 72: // Release Time(リリースタイム)
		ccReleaseTime = uint7ToFloat(value, 0, 1);
		break;
	case 73: // Attack Time(アタックタイム)
		ccAttackTime = uint7ToFloat(value, 0, 1);
		break;
	case 75: // Decay Time(ディケイタイム)
		ccDecayTime = uint7ToFloat(value, 0, 1);
		break;
	// --- チャネルモードメッセージ ---
	case 121: // リセットオールコントローラ
		reset();
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
	if (mRPNDetector.parseControllerMessage(midiChannel, ctrlNo, value, rpn)) {
		if(rpn.isNRPN) {
			mRawNRPNs.insert_or_assign(rpn.parameterNumber, std::make_pair(rpn.value, rpn.is14BitValue));
		}else {
			mRawRPNs.insert_or_assign(rpn.parameterNumber, std::make_pair(rpn.value, rpn.is14BitValue));
		}
		const bool isRPN = !rpn.isNRPN;
		const uint8_t pnMSB = (0x3F80 & rpn.parameterNumber) >> 7;
		const uint8_t pnLSB = (0x007F & rpn.parameterNumber) >> 0;

		// RPN/ NRPN 即時反映系
		// - NRPN(XG) : ドラムパートへ切替
		if(synth.systemType().isXG() && pnMSB == 127) {
			isDrumPart = true;
		}
	}
}
ChannelState::Digest ChannelState::digest(const midi::SystemType& systemType)const
{
	Digest digest;

	digest.ch = midiChannel;
	digest.progId = progId;
	digest.bankSelectMSB = bankSelectMSB;
	digest.bankSelectLSB = bankSelectLSB;
	digest.volume = ccVolume; 
	digest.expression = ccExpression;
	digest.pan = ccPanpot;
	digest.pitchBend = pitchBend(systemType);
	digest.attackTime = ccAttackTime;
	digest.decayTime = ccDecayTime;
	digest.releaseTime = ccReleaseTime;
	digest.pedal = ccPedal;
	digest.drum = isDrumPart;

	return digest;
}

float ChannelState::pitchBend(const midi::SystemType& systemType)const noexcept
{
	return pitchBend(systemType, pitchBendWithoutSensitivity);
}
float ChannelState::pitchBend(const midi::SystemType& systemType, float pitchBendWithoutSensitivity)const noexcept
{
	auto pitchBendSensitivity = static_cast<float>(nrpn_getInt7(0, 0).value_or(synth.systemType().isOnlyGM1() ? 12 : 2));
	return pitchBendWithoutSensitivity * pitchBendSensitivity;
}


// ---

std::optional<int> ChannelState::rpn_getInt14(int msb, int lsb)const noexcept
{
	auto pn = ((msb & 0x7F) << 7) + (lsb & 0x7F);
	auto found = mRawRPNs.find(pn);
	if(found == mRawRPNs.end()) return {};
	auto [value, is14bit] = found->second;
	return (is14bit ? value : ((value & 0x7F) << 7)) - 0x2000;
}
std::optional<int> ChannelState::nrpn_getInt14(int msb, int lsb)const noexcept
{
	auto pn = ((msb & 0x7F) << 7) + (lsb & 0x7F);
	auto found = mRawNRPNs.find(pn);
	if(found == mRawNRPNs.end()) return {};
	auto [value, is14bit] = found->second;
	return (is14bit ? value : ((value & 0x7F) << 7)) - 0x2000;
}

std::optional<int> ChannelState::rpn_getInt7(int msb, int lsb)const noexcept
{
	auto found = rpn_getInt14(msb, lsb);
	return found ? std::make_optional(found.value() / 0x80) : std::nullopt;
}
std::optional<int> ChannelState::nrpn_getInt7(int msb, int lsb)const noexcept
{
	auto found = nrpn_getInt14(msb, lsb);
	return found ? std::make_optional(found.value() / 0x80) : std::nullopt;
}


std::optional<float> ChannelState::rpn_getFloat(int msb, int lsb, float min, float max)const noexcept
{
	auto found = rpn_getInt14(msb, lsb);
	return found ? std::make_optional(int14ToFloat(found.value(), min, max)) : std::nullopt;
}
std::optional<float> ChannelState::nrpn_getFloat(int msb, int lsb, float min, float max)const noexcept
{
	auto found = nrpn_getInt14(msb, lsb);
	return found ? std::make_optional(int14ToFloat(found.value(), min, max)) : std::nullopt;
}
