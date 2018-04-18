#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::MIDI::Synthesizer
{

// トーンジェネレータ(音源モジュール) 基底クラス
class ToneGenerator
	: non_copy
{
public:
	virtual ~ToneGenerator() {}

	// ノートオン
	virtual void noteOn(uint8_t ch, uint8_t noteNo, uint8_t vel) {}

	// ノートオフ
	virtual void noteOff(uint8_t ch, uint8_t noteNo, uint8_t vel) {}

	// コントロールチェンジ
	virtual void controlChange(uint8_t ch, uint8_t ctrlNo, uint8_t value) {}

	// プログラムチェンジ
	virtual void programChange(uint8_t ch, uint8_t progNo) {}

	// ピッチベンド
	virtual void pitchBend(uint8_t ch, int16_t pitch) {}

	// システムエクスクルーシブ
	virtual void sysExMessage(const uint8_t* data, size_t len) {}
};

}