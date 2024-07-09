// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/synth/voice.hpp>
#include <lsp/midi/system_type.hpp>

#include <array>

namespace lsp::synth
{
class LuathSynth;


struct ChannelState final
	: non_copy
{
	struct Digest {
		int ch = 1; // チャネル [1～16]
		int progId = 0; // プログラムID
		int bankSelectMSB = 0; // バンクセレクト
		int bankSelectLSB = 0; // バンクセレクト
		float pan = 0.5f; // パン
		float volume = 1.0f; // チャネルボリューム
		float expression = 1.0f; // エクスプレッション
		float pitchBend = 0.f; // ピッチベンド(ピッチベンドセンシティビティ考慮済み)
		float attackTime = 0.f; // アタックタイム
		float decayTime = 0.f; // ディケイタイム
		float releaseTime = 0.f; // リリースタイム
		bool pedal = false; // ペダルOn/Off
		bool drum = false; // ドラムか否か

	};


	LuathSynth& synth;

	// チャネル番号 (1～16)
	const int midiChannel;
			
	// 音色関連
	int progId; // プログラムId
	int bankSelectMSB; // CC:0  - バンクセレクトMSB
	int bankSelectLSB; // CC:32 - バンクセレクトLSB
	bool isDrumPart = false;

	// コントロールチェンジ
	float ccVolume;			// CC:7 - チャネルボリューム
	float ccPanpot;			// CC:10 - パン [0.0(左), +1.0(右)]
	float ccExpression;		// CC:11 - エクスプレッション [0.0, +1.0]
	bool  ccPedal;			// CC:64 - Hold1(ペダル)
	float ccReleaseTime;	// CC:72 - リリースタイム [0.0 - (0.5:default) - 1.0]
	float ccAttackTime;		// CC:73 - アタックタイム [0.0 - (0.5:default) - 1.0]
	float ccDecayTime;		// CC:75 - ディケイタイム [0.0 - (0.5:default) - 1.0]

	// ピッチベンド
	float pitchBendWithoutSensitivity; // [-1.0 ～ +1.0]
	float pitchBend(const midi::SystemType& systemType)const noexcept;
	float pitchBend(const midi::SystemType& systemType, float pitchBendWithoutSensitivity)const noexcept;


	// RPN and NRPN
	std::optional<int>   rpn_getInt14  (int msb, int lsb)const noexcept; // [-0x2000, +0x1FFF]
	std::optional<int>   rpn_getInt7(int msb, int lsb)const noexcept;    // [-0x80,   +0x7F]
	std::optional<float> rpn_getFloat(int msb, int lsb, float min, float max)const noexcept;


	// ---
	std::optional<int>   nrpn_getInt14(int msb, int lsb)const noexcept;   // [-0x2000, +0x1FFF]
	std::optional<int>   nrpn_getInt7(int msb, int lsb)const noexcept;    // [-0x80,   +0x7F]
	std::optional<float> nrpn_getFloat(int msb, int lsb, float min, float max)const noexcept; 

	ChannelState(LuathSynth& synth, int ch);
	Digest digest(const midi::SystemType& systemType)const;
	void reset();

	// callbacks
	void handleController(int ctrlNo, int value);


private:
	// RPN/NRPN State
	juce::MidiRPNDetector mRPNDetector;
	std::unordered_map<int, std::pair<int, bool>> mRawRPNs;   // [0x0000, 0x3FFF]
	std::unordered_map<int, std::pair<int, bool>> mRawNRPNs;  // [0x0000, 0x3FFF]
};

}