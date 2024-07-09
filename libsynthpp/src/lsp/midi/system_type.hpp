// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>

namespace lsp::midi
{

// MIDI システム種別
class SystemType
{
public:
	static constexpr SystemType GM1()noexcept { SystemType type; type.mIsOnlyGM1 = true;  return type; }
	static constexpr SystemType GM2()noexcept { SystemType type; type.mIsGM2 = true; return type; }
	static constexpr SystemType XG()noexcept  { SystemType type; type.mIsXG = true; return type; }
	static constexpr SystemType GS()noexcept  { SystemType type; type.mIsGS = true; return type; }
	static constexpr SystemType SystemModeSet1()noexcept { SystemType type; type.mIsGS = true; type.mIsSystemModeSet1 = true; return type; }
	static constexpr SystemType SystemModeSet2()noexcept { SystemType type; type.mIsGS = true; type.mIsSystemModeSet2 = true; return type; }

	[[nodiscard]] constexpr bool isOnlyGM1()const noexcept { return mIsOnlyGM1; }
	[[nodiscard]] constexpr bool isGM2()const noexcept { return mIsGM2; }
	[[nodiscard]] constexpr bool isXG()const noexcept { return mIsXG; }
	[[nodiscard]] constexpr bool isGS()const noexcept { return mIsGS; }
	[[nodiscard]] constexpr bool isSystemModeSet1()const noexcept { return mIsSystemModeSet1; }
	[[nodiscard]] constexpr bool isSystemModeSet2()const noexcept { return mIsSystemModeSet2; }

	[[nodiscard]] constexpr const char* toPrintableString()const noexcept {
		if(mIsGM2) return "GM2";
		if(mIsXG) return "XG";
		if(mIsSystemModeSet1) return "System Mode Set 1";
		if(mIsSystemModeSet2) return "System Mode Set 2";
		if(mIsGS) return "GS";
		return "GM1";
	}
	[[nodiscard]] constexpr const wchar_t* toPrintableWString()const noexcept {
		if(mIsGM2) return L"GM2";
		if(mIsXG) return L"XG";
		if(mIsSystemModeSet1) return L"System Mode Set 1";
		if(mIsSystemModeSet2) return L"System Mode Set 2";
		if(mIsGS) return L"GS";
		return L"GM1";
	}

private:
	bool mIsOnlyGM1 = false;
	bool mIsGM2 = false;
	bool mIsXG = false;
	bool mIsGS = false;
	bool mIsSystemModeSet1 = false;
	bool mIsSystemModeSet2 = false;
};


}
