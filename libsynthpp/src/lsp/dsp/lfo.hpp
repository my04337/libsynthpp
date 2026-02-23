// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>

#include <cmath>
#include <numbers>

namespace lsp::dsp
{

// LFO (Low Frequency Oscillator)
// 正弦波ベースの汎用LFO。ビブラート、トレモロ、オートワウ等に使用できます。
// update() は -1.0 ~ +1.0 の生のLFO値を返します。
// 深度やスケーリングは呼び出し側で適用してください。
template<std::floating_point parameter_type = float>
class LFO final
{
public:
	LFO() = default;

	// パラメータを設定します (発振中でもリアルタイムに変更可能)
	// rate: 周波数(Hz), delaySec: 発振開始までの遅延(秒)
	void setParam(parameter_type sampleFreq, parameter_type rate, parameter_type delaySec = 0)noexcept
	{
		mPhaseIncrement = TWO_PI * rate / sampleFreq;
		mDelayTime = static_cast<uint64_t>(delaySec * sampleFreq);
	}

	// 1サンプル進めてLFO値を返します [-1.0, +1.0]
	// ディレイ期間中は0を返しますが、位相は常に進みます
	parameter_type update()noexcept
	{
		// 位相を常に進める (ディレイ中も維持し、有効化時に滑らかに開始する)
		mPhase += mPhaseIncrement;
		if (mPhase >= TWO_PI) {
			mPhase -= TWO_PI;
		}

		++mTime;

		if (mTime <= mDelayTime) {
			return 0;
		}

		return std::sin(mPhase);
	}

	// 内部状態をリセットします (位相・時間カウンタを初期化)
	void reset()noexcept
	{
		mPhase = 0;
		mTime = 0;
	}

	// 現在のLFO値を取得します (状態を進めない)
	parameter_type value()const noexcept
	{
		if (mTime <= mDelayTime) {
			return 0;
		}
		return std::sin(mPhase);
	}

private:
	static constexpr parameter_type TWO_PI = 2 * std::numbers::pi_v<parameter_type>;

	parameter_type mPhase = 0;            // 現在の位相 (ラジアン)
	parameter_type mPhaseIncrement = 0;   // 1サンプルあたりの位相増分
	uint64_t mDelayTime = 0;              // 発振開始までのディレイ (サンプル数)
	uint64_t mTime = 0;                   // リセットからのサンプルカウンタ
};

}
