#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>
#include <LSP/Synth/VoiceMapper.hpp>
#include <LSP/Synth/Voice.hpp>

#include <array>

namespace LSP::Synth
{

class MidiChannel
{
public:
	struct Info {
		uint8_t ch; // チャネル
		uint8_t programChange; // プログラムチェンジ
		uint8_t bankSelect; // バンクセレクト
		float pan; // パン
		float volume; // チャネルボリューム
		float expression; // エクスプレッション
	};

	MidiChannel(uint32_t sampleFreq, uint8_t ch);

	void reset(LSP::MIDI::SystemType type);
	void resetParameterNumberState();
	// ---
	void noteOn(uint32_t noteNo, uint8_t vel);
	void noteOff(uint32_t noteNo);
	void programChange(uint8_t progId);
	void controlChange(uint8_t ctrlNo, uint8_t value);
	void holdOn();
	void holdOff();
	// ---
	std::pair<float,float> update();
	// ---
	Info info()const;
	// ---


	// サンプリング周波数(実行時に動的にセット)
	const uint32_t sampleFreq;
	// チャネル番号(実行時に動的にセット)
	const uint8_t ch;
		
	// プログラムチェンジ
	uint8_t pcId; // プログラムId
	uint16_t bankSelect; // バンクセレクト
	LSP::Filter::EnvelopeGenerator<float> pcEG; // チャネルEG(パラメータ計算済)
	void updateProgram();

	// コントロールチェンジ
	uint8_t ccPrevCtrlNo;
	uint8_t ccPrevValue;
	uint8_t ccBankSelectMSB;// CC:0 - バンクセレクトMSB
	float ccVolume;			// CC:7 - チャネル簿リュ－無;
	float ccPan;			// CC:10 - パン [0.0(左), +1.0(右)]
	float ccExpression;		// CC:11 - エクスプレッション [0.0, +1.0]
	uint8_t ccBankSelectLSB;// CC:32 - バンクセレクトLSB

	// RPN/NRPN State
	std::optional<uint8_t> ccRPN_MSB;
	std::optional<uint8_t> ccRPN_LSB;
	std::optional<uint8_t> ccNRPN_MSB;
	std::optional<uint8_t> ccNRPN_LSB;
	std::optional<uint8_t> ccDE_MSB;
	std::optional<uint8_t> ccDE_LSB;

	// RPN
	int16_t rpnPitchBendSensitibity;
	bool    rpnNull;

private:
	void voice_noteOn(VoiceId id, uint32_t noteNo, uint8_t vel);
	void voice_noteOff(VoiceId id);

private:
	// 発音状態管理
	LSP::Synth::VoiceMapper _voiceMapper;
	// ボイス生成
	std::unordered_map<VoiceId, std::unique_ptr<LSP::Synth::Voice>> _voices;
};

}