#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::MIDI
{
class Message;

// シーケンサ 抽象クラス
class Sequencer
	: non_copy
{
public:
	virtual ~Sequencer() {}

	// ノートオン
	virtual void noteOn(uint8_t ch, uint8_t noteNo, uint8_t vel) = 0;

	// ノートオフ
	virtual void noteOff(uint8_t ch, uint8_t noteNo, uint8_t vel) = 0;

	// コントロールチェンジ
	virtual void controlChange(uint8_t ch, uint8_t ctrlNo, uint8_t value) = 0;

	// プログラムチェンジ
	virtual void programChange(uint8_t ch, uint8_t progNo) = 0;

	// ピッチベンド
	virtual void pitchBend(uint8_t ch, int16_t pitch) = 0;

	// システムエクスクルーシブ
	virtual void sysExMessage(const uint8_t* data, size_t len) = 0;
};


// シーケンサ : 何もしない(試験用)
class NullSequencer 
	: public Sequencer
{
public:
	virtual void noteOn(uint8_t ch, uint8_t noteNo, uint8_t vel)override {}
	virtual void noteOff(uint8_t ch, uint8_t noteNo, uint8_t vel)override {}
	virtual void controlChange(uint8_t ch, uint8_t ctrlNo, uint8_t value)override {}
	virtual void programChange(uint8_t ch, uint8_t progNo)override {}
	virtual void pitchBend(uint8_t ch, int16_t pitch)override {}
	virtual void sysExMessage(const uint8_t* data, size_t len)override {}
};

}