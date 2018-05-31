#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Synthesizer/Tone.hpp>

namespace LSP::MIDI::Synthesizer
{

// ノートOn/Off情報から各トーンへマッピングする (スレッドセーフクラス)
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
	mutable std::mutex mMutex;
	ToneMap mTones;
	bool mHold=false; // ダンパーペダル 状態
};

}