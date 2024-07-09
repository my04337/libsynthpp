// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <luath-app/core/core.hpp>
#include <lsp/synth/voice.hpp>

namespace luath::app
{

// MIDIチャネルの標準色を取得します
constexpr juce::Colour getMidiChannelColor(int ch)
{
	switch(ch) {
	case  1: return juce::Colour::fromFloatRGBA(1.f, 0.f, 0.f, 1.f); // 赤
	case  2: return juce::Colour::fromFloatRGBA(1.f, 0.5f, 0.f, 1.f); // 朱色
	case  3: return juce::Colour::fromFloatRGBA(1.f, 0.75f, 0.f, 1.f); // ゴールデンイエロー
	case  4: return juce::Colour::fromFloatRGBA(1.f, 1.f, 0.f, 1.f); // 黄色
	case  5: return juce::Colour::fromFloatRGBA(0.75f, 1.f, 0.f, 1.f); // 明るい黄緑色 
	case  6: return juce::Colour::fromFloatRGBA(0.f, 1.f, 0.f, 1.f); // 緑
	case  7: return juce::Colour::fromFloatRGBA(0.f, 1.f, 0.75f, 1.f); // 黄緑色
	case  8: return juce::Colour::fromFloatRGBA(0.f, 0.75f, 1.f, 1.f); // セルリアンブルー
	case  9: return juce::Colour::fromFloatRGBA(0.f, 0.3f, 1.f, 1.f); // コバルトブルー
	case 10: return juce::Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 1.f); // グレー ※通常ドラム
	case 11: return juce::Colour::fromFloatRGBA(0.3f, 0.f, 1.f, 1.f); // ヒヤシンス
	case 12: return juce::Colour::fromFloatRGBA(0.5f, 0.f, 1.f, 1.f); // バイオレット
	case 13: return juce::Colour::fromFloatRGBA(0.75f, 0.f, 1.f, 1.f); // ムラサキ
	case 14: return juce::Colour::fromFloatRGBA(1.f, 0.f, 1.f, 1.f); // マゼンタ
	case 15: return juce::Colour::fromFloatRGBA(1.f, 0.f, 0.5f, 1.f); // ルビーレッド
	case 16: return juce::Colour::fromFloatRGBA(0.75f, 0.f, 0.3f, 1.f); // カーマイン
	}
	std::unreachable();
};

// ボイスの状態をテキストに変換します
constexpr const wchar_t* state2text(lsp::synth::LuathVoice::EnvelopeState state)
{
	switch(state) {
	case lsp::synth::LuathVoice::EnvelopeState::Attack: return L"Attack";
	case lsp::synth::LuathVoice::EnvelopeState::Hold:   return L"Hold";
	case lsp::synth::LuathVoice::EnvelopeState::Decay:  return L"Decay";
	case lsp::synth::LuathVoice::EnvelopeState::Fade:   return L"Fade";
	case lsp::synth::LuathVoice::EnvelopeState::Release:return L"Release";
	case lsp::synth::LuathVoice::EnvelopeState::Free:   return L"Free";
	default: return L"Unknown";
	}
};

// 周波数から近しい音階を取得します
std::wstring freq2scale(float freq);

}// namespace luath::app