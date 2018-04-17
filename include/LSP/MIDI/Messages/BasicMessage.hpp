#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>

namespace LSP::MIDI::Messages
{

// チャネルボイスメッセージ : 特定チャネル向けメッセージ
class ChannelVoiceMessage
	: public Message
{
public:
	// 対象チャネルを取得します
	virtual uint8_t channel()const noexcept = 0;
};

/// ノートオン
class NoteOn
	: public ChannelVoiceMessage
{
public:
	NoteOn(uint8_t ch, uint8_t noteNo, uint8_t vel)
		: mChannel(ch), mNoteNo(noteNo), mVelocity(vel)
	{}

	virtual uint8_t channel()const noexcept final { return mChannel; } 
	virtual void play(Sequencer& seq)const override;

private:
	uint8_t mChannel;
	uint8_t mNoteNo;
	uint8_t mVelocity;
};

/// ノートオフ
class NoteOff
	: public ChannelVoiceMessage
{
public:
	NoteOff(uint8_t ch, uint8_t noteNo, uint8_t vel)
		: mChannel(ch), mNoteNo(noteNo), mVelocity(vel)
	{}

	virtual uint8_t channel()const noexcept final { return mChannel; } 
	virtual void play(Sequencer& seq)const override;

private:
	uint8_t mChannel;
	uint8_t mNoteNo;
	uint8_t mVelocity;
};

/// ポリフォニックキープレッシャー (アフタータッチ)
class PolyphonicKeyPressure
	: public ChannelVoiceMessage
{
public:
	PolyphonicKeyPressure(uint8_t ch, uint8_t noteNo, uint8_t value)
		: mChannel(ch), mNoteNo(noteNo), mValue(value)
	{}

	virtual uint8_t channel()const noexcept final { return mChannel; } 
	virtual void play(Sequencer& seq)const override {} // TODO 未実装

private:
	uint8_t mChannel;
	uint8_t mNoteNo;
	uint8_t mValue;
};

/// コントロールチェンジ & チャネルモードメッセージ
class ControlChange
	: public ChannelVoiceMessage
{
public:
	ControlChange(uint8_t ch, uint8_t ctrlNo, uint8_t value)
		: mChannel(ch), mCtrlNo(ctrlNo), mValue(value)
	{}

	virtual uint8_t channel()const noexcept final { return mChannel; } 
	virtual void play(Sequencer& seq)const override;

private:
	uint8_t mChannel;
	uint8_t mCtrlNo;
	uint8_t mValue;
};

/// プログラムチェンジ
class ProgramChange
	: public ChannelVoiceMessage
{
public:
	ProgramChange(uint8_t ch, uint8_t progNo)
		: mChannel(ch), mProgNo(progNo)
	{}

	virtual uint8_t channel()const noexcept final { return mChannel; } 
	virtual void play(Sequencer& seq)const override;

private:
	uint8_t mChannel;
	uint8_t mProgNo;
};

/// チャネルプレッシャー (アフタータッチ)
class ChannelPressure
	: public ChannelVoiceMessage
{
public:
	ChannelPressure(uint8_t ch, uint8_t value)
		: mChannel(ch), mValue(value)
	{}

	virtual uint8_t channel()const noexcept final { return mChannel; } 
	virtual void play(Sequencer& seq)const override {} // TODO 未実装

private:
	uint8_t mChannel;
	uint8_t mValue;
};

/// ピッチベンド
class PitchBend
	: public ChannelVoiceMessage
{
public:
	PitchBend(uint8_t ch, int16_t pitch)
		: mChannel(ch), mPitch(pitch)
	{}

	virtual uint8_t channel()const noexcept final { return mChannel; } 
	virtual void play(Sequencer& seq)const override;

private:
	uint8_t mChannel;
	int16_t mPitch; // -8192 <= x < +8191
};

}