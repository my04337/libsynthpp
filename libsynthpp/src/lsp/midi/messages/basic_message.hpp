#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/message.hpp>

namespace lsp::midi::messages
{

/// ノートオン
class NoteOn
	: public ChannelVoiceMessage
{
public:
	NoteOn(uint8_t ch, uint8_t noteNo, uint8_t vel)
		: mChannel(ch), mNoteNo(noteNo), mVelocity(vel)
	{}

	uint8_t channel()const noexcept override final { return mChannel; } 
	uint8_t noteNo()const noexcept { return mNoteNo; }
	uint8_t velocity()const noexcept { return mVelocity; }

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

	uint8_t channel()const noexcept override final { return mChannel; } 
	uint8_t noteNo()const noexcept { return mNoteNo; }

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

	uint8_t channel()const noexcept override final { return mChannel; } 

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

	uint8_t channel()const noexcept override final { return mChannel; } 
	uint8_t ctrlNo()const noexcept { return mCtrlNo; }
	uint8_t value()const noexcept { return mValue; }

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

	uint8_t channel()const noexcept override final { return mChannel; }
	uint8_t progId()const noexcept { return mProgNo; }

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

	uint8_t channel()const noexcept override final { return mChannel; } 

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

	uint8_t channel()const noexcept override final { return mChannel; } 
	int16_t pitch()const noexcept { return mPitch; }

private:
	uint8_t mChannel;
	int16_t mPitch; // -8192 <= x < +8191
};

}