// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>
#include <lsp/midi/system_type.hpp>

namespace lsp::synth
{

// メロディ用波形種別
enum class MelodyWaveForm
{
	Square,    // 矩形波（バンドリミテッド） ※デフォルト
	Sine,      // 正弦波
	Triangle,  // 三角波
	Sawtooth,  // のこぎり波
	Noise,     // ノイズ
};

// インストゥルメントテーブル用システム種別
// ※ SystemModeSet1/2 は共に GS として扱う
enum class InstrumentSystemType : uint8_t
{
	None = 0, // 共通 (システム種別未指定)
	GM1,
	GM2,
	GS,       // GS / System Mode Set 1 / System Mode Set 2
	XG,
};

// midi::SystemType → InstrumentSystemType 変換
// ※ SystemModeSet1/2 は共に GS として扱う
[[nodiscard]] constexpr InstrumentSystemType toInstrumentSystemType(const midi::SystemType& st) noexcept
{
	if(st.isXG())  return InstrumentSystemType::XG;
	if(st.isGS())  return InstrumentSystemType::GS;
	if(st.isGM2()) return InstrumentSystemType::GM2;
	if(st.isOnlyGM1()) return InstrumentSystemType::GM1;
	return InstrumentSystemType::None;
}

// メロディ音色パラメータ
struct MelodyParam
{
	std::string caption; // GM1音色名

	float volume   = 1.00f; // 音量調整
	float attack   = 0.02f; // sec
	float hold     = 0.00f; // sec
	float decay    = 3.00f; // sec
	float sustain  = 0.00f; // level
	float fade     = 0.00f; // Linear : level/sec, Exp : dBFS/sec
	float release  = 1.00f; // sec

	MelodyWaveForm waveForm = MelodyWaveForm::Square; // 波形種別

	bool isDrumLike  = false; // ドラム風楽器か
	float noteOffset = 0.0f;  // ノートオフセット(半音単位)
};

// ドラム音色パラメータ
struct DrumParam
{
	int32_t pitch = 69;   // NoteNo
	float volume  = 1.0f; // 音量調整
	float attack  = 0.0f; // sec
	float hold    = 0.0f; // sec
	float decay   = 0.3f; // sec
	float pan     = 0.5f; // 0～1
};

// インストゥルメント検索キー (システム種別 + バンクセレクト)
struct InstrumentVariantKey
{
	InstrumentSystemType systemType = InstrumentSystemType::None;
	uint8_t bankMSB = 0;
	uint8_t bankLSB = 0;

	bool operator==(const InstrumentVariantKey&) const = default;
};

}

// std::hash 特殊化
template<>
struct std::hash<lsp::synth::InstrumentVariantKey>
{
	size_t operator()(const lsp::synth::InstrumentVariantKey& k) const noexcept
	{
		size_t h = static_cast<size_t>(k.systemType);
		h ^= static_cast<size_t>(k.bankMSB) << 8;
		h ^= static_cast<size_t>(k.bankLSB) << 16;
		return h;
	}
};

namespace lsp::synth
{

// インストゥルメント情報テーブル
class InstrumentTable final
{
public:
	InstrumentTable() = default;

	// メロディパラメータの設定・取得
	void setMelodyParam(const InstrumentVariantKey& variant, int32_t progId, MelodyParam param)
	{
		mMelodyParams[variant][progId] = std::move(param);
	}
	const MelodyParam* findMelodyParam(InstrumentSystemType systemType, uint8_t bankMSB, uint8_t bankLSB, int32_t progId) const
	{
		using ST = InstrumentSystemType;
		// 1. 完全一致 (systemType + bank)
		if(auto p = findMelodyExact({systemType, bankMSB, bankLSB}, progId)) return p;
		// 2. systemType + bank 0/0
		if(bankMSB != 0 || bankLSB != 0) {
			if(auto p = findMelodyExact({systemType, 0, 0}, progId)) return p;
		}
		// 3. 共通 + bank
		if(systemType != ST::None) {
			if(auto p = findMelodyExact({ST::None, bankMSB, bankLSB}, progId)) return p;
		}
		// 4. 共通 + bank 0/0 (デフォルト)
		if(systemType != ST::None || bankMSB != 0 || bankLSB != 0) {
			if(auto p = findMelodyExact({ST::None, 0, 0}, progId)) return p;
		}
		return nullptr;
	}

	// ドラムパラメータの設定・取得
	void setDrumParam(const InstrumentVariantKey& variant, int32_t noteNo, DrumParam param)
	{
		mDrumParams[variant][noteNo] = std::move(param);
	}
	const DrumParam* findDrumParam(InstrumentSystemType systemType, uint8_t bankMSB, uint8_t bankLSB, int32_t noteNo) const
	{
		using ST = InstrumentSystemType;
		// 1. 完全一致 (systemType + bank)
		if(auto p = findDrumExact({systemType, bankMSB, bankLSB}, noteNo)) return p;
		// 2. systemType + bank 0/0
		if(bankMSB != 0 || bankLSB != 0) {
			if(auto p = findDrumExact({systemType, 0, 0}, noteNo)) return p;
		}
		// 3. 共通 + bank
		if(systemType != ST::None) {
			if(auto p = findDrumExact({ST::None, bankMSB, bankLSB}, noteNo)) return p;
		}
		// 4. 共通 + bank 0/0 (デフォルト)
		if(systemType != ST::None || bankMSB != 0 || bankLSB != 0) {
			if(auto p = findDrumExact({ST::None, 0, 0}, noteNo)) return p;
		}
		return nullptr;
	}

private:
	const MelodyParam* findMelodyExact(const InstrumentVariantKey& key, int32_t progId) const
	{
		auto varIt = mMelodyParams.find(key);
		if(varIt == mMelodyParams.end()) return nullptr;
		auto it = varIt->second.find(progId);
		return it != varIt->second.end() ? &it->second : nullptr;
	}
	const DrumParam* findDrumExact(const InstrumentVariantKey& key, int32_t noteNo) const
	{
		auto varIt = mDrumParams.find(key);
		if(varIt == mDrumParams.end()) return nullptr;
		auto it = varIt->second.find(noteNo);
		return it != varIt->second.end() ? &it->second : nullptr;
	}

	std::unordered_map<InstrumentVariantKey, std::unordered_map<int32_t, MelodyParam>> mMelodyParams;
	std::unordered_map<InstrumentVariantKey, std::unordered_map<int32_t, DrumParam>> mDrumParams;
};

}
