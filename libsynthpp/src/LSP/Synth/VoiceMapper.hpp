#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>
#include <LSP/Synth/Voice.hpp>

namespace LSP::Synth
{
// ノートOn/Off情報から各ボイスへマッピングする
class VoiceMapper
	: non_copy
{
public:
	// 現在の同時発音数を取得します
	size_t count()const noexcept;

	// 発音状態をリセットします (コールバックは無し)
	void reset();

	// ノート オン (vel > 0)
	std::pair</*on*/VoiceId, /*off*/VoiceId> noteOn(uint32_t noteNo);
	// ノート オフ
	VoiceId noteOff(uint32_t noteNo, bool force=false);
	// ダンパーペダル オン
	void holdOn();
	// ダンパーペダル オフ
	std::vector</*off*/VoiceId> holdOff();

private:
	struct NoteInfo {
		VoiceId toneId;			// トーンId
		bool holding = false;	// ノートオフ後でもダンパーペダルによって保持されている
	};
	using VoiceMap = std::unordered_map<uint32_t, NoteInfo>;

	VoiceId _noteOn(uint32_t noteNo);
	VoiceId _noteOff(uint32_t noteNo);

private:
	VoiceMap mVoices;
	bool mHold=false; // ダンパーペダル 状態
};


}