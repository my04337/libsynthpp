#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>

namespace LSP::MIDI::Synthesizer
{

struct _tone_id_tag {};
using ToneId = issuable_id_base_t<_tone_id_tag>;

// ノートOn/Off情報から各トーンへマッピングする
class ToneMapper
	: non_copy
{
public:
	using Callback = std::function<void(ToneId toneNo, uint32_t noteNo, uint8_t vel)>;

public:
	void setCallback(Callback callback);

	// 発音状態をリセットします (コールバックは無し)
	void reset();

	// 同時発音数を取得します
	size_t count()const noexcept;

	// ノート オン (vel > 0)
	ToneId noteOn(uint32_t noteNo, uint8_t vel);
	// ノート オフ
	void noteOff(uint32_t noteNo, bool force=false);
	// ダンパーペダル オン
	void holdOn();
	// ダンパーペダル オフ
	void holdOff();
	
private:
	struct NoteInfo {
		ToneId toneId;			// トーンId
		bool holding = false;	// ノートオフ後でもダンパーペダルによって保持されている
	};
	using ToneMap = std::unordered_map<uint32_t, NoteInfo>;

	ToneId _noteOn(uint32_t noteNo, uint8_t vel);
	void _noteOff(uint32_t noteNo);
	ToneMap::iterator _noteOff(ToneMap::iterator iter);
	void _setHold(uint32_t noteNo);
	void _resetHold();

private:

	Callback mCallback;
	ToneMap mTones;
	bool mHold=false; // ダンパーペダル 状態
};

}