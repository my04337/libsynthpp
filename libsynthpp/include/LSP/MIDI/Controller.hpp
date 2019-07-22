#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>
#include <LSP/MIDI/Message.hpp>

namespace LSP::MIDI
{
// トーン識別番号
struct _tone_id_tag {};
using ToneId = issuable_id_base_t<_tone_id_tag>;

// ノートOn/Off情報から各トーンへマッピングする
class ToneMapper
	: non_copy
{
public:
	// 現在の同時発音数を取得します
	size_t count()const noexcept;

	// 発音状態をリセットします (コールバックは無し)
	void reset();

	// ノート オン (vel > 0)
	std::pair</*on*/ToneId, /*off*/ToneId> noteOn(uint32_t noteNo);
	// ノート オフ
	ToneId noteOff(uint32_t noteNo, bool force=false);
	// ダンパーペダル オン
	void holdOn();
	// ダンパーペダル オフ
	std::vector</*off*/ToneId> holdOff();

private:
	struct NoteInfo {
		ToneId toneId;			// トーンId
		bool holding = false;	// ノートオフ後でもダンパーペダルによって保持されている
	};
	using ToneMap = std::unordered_map<uint32_t, NoteInfo>;

	ToneId _noteOn(uint32_t noteNo);
	ToneId _noteOff(uint32_t noteNo);

private:
	ToneMap mTones;
	bool mHold=false; // ダンパーペダル 状態
};


// MIDIシンセサイザ コントローラ
class Controller
	: non_copy_move
{
public:
	virtual ~Controller() {}

	// MIDIメッセージ受信コールバック : メッセージ類は蓄積される
	virtual void onMidiMessageReceived(clock::time_point msg_time, const std::shared_ptr<const Message>& msg) = 0;
};


}