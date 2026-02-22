// SPDX-FileCopyrightText: 2018 my04337
// SPDX-License-Identifier: MIT

#pragma once

#include <lsp/core/core.hpp>

namespace lsp::dsp
{


enum class EnvelopeState
{
	Attack,		// アタック中 : ノートオン直後、音量を0から最大音量へ上げるフェーズ
	Hold,		// ホールド中 : アタック後、最大音量を維持するフェーズ
	Decay,		// ディケイ中 : ホールド後、音量をサスティンレベルまで下げるフェーズ
	Fade,		// フェード中 : ディケイレベルに達した後、フェードスロープに従い徐々に音量を下げるフェーズ
	Release,	// リリース中 : ノートオフ後、音量を0に向けて下げていくフェーズ
	Free,		// 止音中 : エンベロープジェネレータは特に稼働しておらず、常に音量0を出力する
};

// エンベロープ カーブ形状
enum class EnvelopeCurveShape
{
	Linear,	// 線形な変化 : 管楽器など向け
	Exp,	// 指数的変化 : 打楽器など向け
};

// エンベロープ カーブパラメータ
template<std::floating_point parameter_type>
struct EnvelopeCurve
{
	EnvelopeCurve() 
		: shape(EnvelopeCurveShape::Linear)
		, exp_param_n(1)
		, exp_param_level_at_n(0)
	{}
	EnvelopeCurve(parameter_type exp_param_n)
		: shape(EnvelopeCurveShape::Exp)
		, exp_param_n(exp_param_n)
		, exp_param_level_at_n(static_cast<parameter_type>(1 - std::exp(-exp_param_n)))
	{}

	constexpr operator EnvelopeCurveShape()const noexcept { return shape; }

	EnvelopeCurveShape shape;
	parameter_type exp_param_n;				// Exp : 時定数τ=1[秒]としたときの変化期間[秒]
	parameter_type exp_param_level_at_n;	// Exp : n[秒]地点でのレベル
};


// メロディパート用 AHDSFRエンベロープジェネレータ
template<std::floating_point parameter_type>
class MelodyEnvelopeGenerator final
{
public:
	using Curve = EnvelopeCurve<parameter_type>;
	using Shape = EnvelopeCurveShape;

	MelodyEnvelopeGenerator()
	{
		reset();
	}
	void reset() {
		switchToFree();
	}

	// エンベロープ形状パラメータを指定します (簡易版)
	void setEnvelope(
		float sampleFreq,				// Hz
		float attack_time,				// sec
		float decay_time,				// sec
		parameter_type sustain_level,	// level (0 <= x <= 1)
		float release_time				// sec
	)
	{
		setEnvelope(sampleFreq, {}, attack_time, 0, decay_time, sustain_level, 0, release_time, std::numeric_limits<parameter_type>::epsilon());
	}
	// エンベロープ形状パラメータを指定します
	void setEnvelope(
		float sampleFreq,				// Hz
		Curve curve,					
		float attack_time,				// sec
		float hold_time,				// sec
		float decay_time,				// sec
		parameter_type sustain_level,	// level
		float fade_slope,				// Linear : level/sec, Exp : dBFS/sec
		float release_time,				// sec
		parameter_type threshold_level	// level
	)
	{
		mCurve = curve;
		mAttackTime   = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * attack_time));
		mHoldTime     = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * hold_time));
		mDecayTime    = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * decay_time));
		mReleaseTime  = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * release_time));
		mSustainLevel = std::clamp<parameter_type>(sustain_level, 0, 1);
		mFadeSlope    = std::min<parameter_type>(fade_slope, 0) / sampleFreq;
		mThresholdLevel = std::clamp<parameter_type>(threshold_level, 0, 1);
	}

	void noteOn()
	{
		switchToAttack();
	}

	void noteOff()
	{
		switchToRelease(envelope());
	}

	// リリースタイムを動的に更新します
	// Release状態中に呼ばれた場合、現在の進行度を維持するよう時間を補正します
	void setReleaseTime(float sampleFreq, float release_time)
	{
		auto newReleaseTime = std::max<uint64_t>(1, static_cast<uint64_t>(sampleFreq * release_time));

		if (mState == EnvelopeState::Release && mReleaseTime > 0) {
			auto progress = static_cast<parameter_type>(mTime) / static_cast<parameter_type>(mReleaseTime);
			mTime = static_cast<uint64_t>(std::clamp(progress, static_cast<parameter_type>(0), static_cast<parameter_type>(1)) * newReleaseTime);
		}

		mReleaseTime = newReleaseTime;
	}

	parameter_type envelope()const
	{
		switch (mState) {
		case EnvelopeState::Attack:  return easing(mAttackTime);
		case EnvelopeState::Hold:    return easing(mHoldTime);
		case EnvelopeState::Decay:   return easing(mDecayTime);
		case EnvelopeState::Release: return easing(mReleaseTime);
		case EnvelopeState::Fade:    return easingSlope();
		case EnvelopeState::Free:    return 0;
		}
		std::unreachable();
	}

	parameter_type update()
	{
		auto v = envelope();
		++mTime;

		switch (mState) {
		case EnvelopeState::Attack:
			if (mTime < mAttackTime) {
				break;
			} else {
				mTime -= mAttackTime;
				switchToHold();
				[[fallthrough]];
			}
		case EnvelopeState::Hold:
			if (mTime < mHoldTime) {
				break;
			} else {
				mTime -= mHoldTime;
				switchToDecay();
				[[fallthrough]];
			}
		case EnvelopeState::Decay:
			if (mTime < mDecayTime) {
				break;
			} else {
				mTime -= mDecayTime;
				switchToFade();
			}
			[[fallthrough]];
		case EnvelopeState::Fade:
			if (v <= mThresholdLevel) {
				switchToFree();
			}
			break;
		case EnvelopeState::Release: 
			if(mTime >= mReleaseTime || v <= mThresholdLevel) {
				switchToFree();
			}
			break;
		case EnvelopeState::Free:
			break;
		}
		return v;
	}

	bool isBusy()const noexcept { return mState != EnvelopeState::Free; }
	EnvelopeState state()const noexcept { return mState; }

private:
	parameter_type easing(uint64_t max)const
	{
		if(max == 0) return mEndLevel;
		const auto p = parameter_type(mTime) / parameter_type(max);
		switch(mCurve) {
		case Shape::Linear: 
			return mBeginLevel * (1-p) + mEndLevel * p;
		case Shape::Exp: {
			auto v = (1 - std::exp(- p * mCurve.exp_param_n)) / mCurve.exp_param_level_at_n;
			return mBeginLevel + (mEndLevel - mBeginLevel) * v;
		}
		}
		std::unreachable();
	}
	parameter_type easingSlope()const
	{
		switch(mCurve) {
		case Shape::Linear: 
			return std::max<parameter_type>(0, mBeginLevel + mFadeSlope * mTime);
		case Shape::Exp:
			return static_cast<parameter_type>(mBeginLevel * std::pow(10, mFadeSlope/20 * mTime));
		}
		std::unreachable();
	}

	void switchToAttack() 
	{
		mState = EnvelopeState::Attack;
		mBeginLevel = 0; mEndLevel = 1;
		mTime = 0;
	}
	void switchToHold() 
	{
		mState = EnvelopeState::Hold;
		mBeginLevel = 1; mEndLevel = 1;
	}
	void switchToDecay() 
	{
		mState = EnvelopeState::Decay;
		mBeginLevel = 1; mEndLevel = mSustainLevel;
	}
	void switchToFade() 
	{
		mState = EnvelopeState::Fade;
		mBeginLevel = mSustainLevel; mEndLevel = 0;
	}
	void switchToRelease(parameter_type current_level) 
	{
		if(mState == EnvelopeState::Free) return;
		mState = EnvelopeState::Release;
		mBeginLevel = current_level; mEndLevel = 0;
		mTime = 0;
	}
	void switchToFree() 
	{
		mState = EnvelopeState::Free;
		mBeginLevel = 0; mEndLevel = 0;
	}

	EnvelopeState mState;
	uint64_t mTime;
	Curve mCurve;
	parameter_type mBeginLevel;
	parameter_type mEndLevel;

	uint64_t mAttackTime;
	uint64_t mHoldTime;
	uint64_t mDecayTime;
	parameter_type mSustainLevel;
	parameter_type mFadeSlope;
	uint64_t mReleaseTime;
	parameter_type mThresholdLevel;
};


// ドラムパート用 AHDエンベロープジェネレータ
// ノートオフを無視し、Decay中に毎サンプルthresholdチェックを行い早期止音する
template<std::floating_point parameter_type>
class DrumEnvelopeGenerator final
{
public:
	using Curve = EnvelopeCurve<parameter_type>;
	using Shape = EnvelopeCurveShape;

	DrumEnvelopeGenerator()
	{
		reset();
	}
	void reset() {
		switchToFree();
	}

	// エンベロープ形状パラメータを指定します
	void setEnvelope(
		float sampleFreq,				// Hz
		Curve curve,
		float attack_time,				// sec
		float hold_time,				// sec
		float decay_time,				// sec
		parameter_type threshold_level	// level
	)
	{
		mCurve = curve;
		mAttackTime = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * attack_time));
		mHoldTime = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * hold_time));
		mDecayTime = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * decay_time));
		mThresholdLevel = std::clamp<parameter_type>(threshold_level, 0, 1);
	}

	void noteOn()
	{
		switchToAttack();
	}

	// ドラムはノートオフを無視する
	void noteOff() {}

	parameter_type envelope()const
	{
		switch (mState) {
		case EnvelopeState::Attack: return easing(mAttackTime);
		case EnvelopeState::Hold:   return easing(mHoldTime);
		case EnvelopeState::Decay:  return easing(mDecayTime);
		case EnvelopeState::Free:   return 0;
		default: return 0; // Fade/Release はドラムでは使用しない
		}
		std::unreachable();
	}

	parameter_type update()
	{
		auto v = envelope();
		++mTime;

		switch (mState) {
		case EnvelopeState::Attack:
			if (mTime < mAttackTime) {
				break;
			} else {
				mTime -= mAttackTime;
				switchToHold();
				[[fallthrough]];
			}
		case EnvelopeState::Hold:
			if (mTime < mHoldTime) {
				break;
			} else {
				mTime -= mHoldTime;
				switchToDecay();
				[[fallthrough]];
			}
		case EnvelopeState::Decay:
			// Decay中でも毎サンプルthresholdチェックを行い、閾値以下で即座に止音する
			// これにより長いDecay時間が設定されても不要に長い発音を防ぐ
			if (v <= mThresholdLevel) {
				switchToFree();
			} else if (mTime >= mDecayTime) {
				// Decay時間経過 → 止音
				switchToFree();
			}
			break;
		case EnvelopeState::Free:
			break;
		default:
			break;
		}
		return v;
	}

	bool isBusy()const noexcept { return mState != EnvelopeState::Free; }
	EnvelopeState state()const noexcept { return mState; }

private:
	parameter_type easing(uint64_t max)const
	{
		if(max == 0) return mEndLevel;
		const auto p = parameter_type(mTime) / parameter_type(max);
		switch(mCurve) {
		case Shape::Linear: 
			return mBeginLevel * (1-p) + mEndLevel * p;
		case Shape::Exp: {
			auto v = (1 - std::exp(- p * mCurve.exp_param_n)) / mCurve.exp_param_level_at_n;
			return mBeginLevel + (mEndLevel - mBeginLevel) * v;
		}
		}
		std::unreachable();
	}

	void switchToAttack() 
	{
		mState = EnvelopeState::Attack;
		mBeginLevel = 0; mEndLevel = 1;
		mTime = 0;
	}
	void switchToHold() 
	{
		mState = EnvelopeState::Hold;
		mBeginLevel = 1; mEndLevel = 1;
	}
	void switchToDecay() 
	{
		mState = EnvelopeState::Decay;
		mBeginLevel = 1; mEndLevel = 0;
	}
	void switchToFree() 
	{
		mState = EnvelopeState::Free;
		mBeginLevel = 0; mEndLevel = 0;
	}

	EnvelopeState mState;
	uint64_t mTime;
	Curve mCurve;
	parameter_type mBeginLevel;
	parameter_type mEndLevel;

	uint64_t mAttackTime;
	uint64_t mHoldTime;
	uint64_t mDecayTime;
	parameter_type mThresholdLevel;
};

// 後方互換性のためのエイリアス
template<std::floating_point parameter_type>
using EnvelopeGenerator = MelodyEnvelopeGenerator<parameter_type>;


}