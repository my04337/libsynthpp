#pragma once

#include <lsp/base/base.hpp>
#include <lsp/base/sample.hpp>

namespace lsp::effector
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

// AHDSFRエンベロープジェネレータ
template<
	floating_sample_typeable parameter_type
>
class EnvelopeGenerator final
{
public:
	// エンベロープモデル
	enum class Model
	{
		Melody,	// メロディパート用 : AHDFSR
		Drum,	// ドラムパート用 : ADR ※ノートオフ無視
	}; 
	// エンベロープ カーブ形状
	enum class Shape
	{
		Linear,	// 線形な変化 : 管楽器など向け
		Exp,	// 指数的変化 : 打楽器など向け
	};
	// エンベロープ カーブパラメータ
	struct Curve
	{
		Curve() 
			: shape(Shape::Linear)
			, exp_param_n(1)
			, exp_param_level_at_n(0)
		{}
		Curve(parameter_type exp_param_n)
			: shape(Shape::Exp)
			, exp_param_n(exp_param_n)
			, exp_param_level_at_n(static_cast<parameter_type>(1 - std::exp(-exp_param_n)))
		{}

		constexpr operator Shape()const noexcept { return shape; }

		Shape shape;	
		parameter_type exp_param_n;				// Exp : 時定数τ=1[秒]としたときの変化期間[秒]
		parameter_type exp_param_level_at_n;	// Exp : n[秒]地点でのレベル
	};
	constexpr static Curve LinearCurve()noexcept 
	{
		return {}; 
	}
	static Curve ExpCurve(parameter_type n)noexcept 
	{ 
		Curve c;
		c.shape = Shape::Exp;
		c.exp_param_n = n;
		c.exp_param_level_at_n = static_cast<parameter_type>(1 - std::exp(-n));
		return c;
	}


public:
	EnvelopeGenerator()
	{
		reset();
	}
	// 状態を初期化します
	void reset() {
		switchToFree();
	}

	// エンベロープ形状パラメータを指定します (メロディ用)
	void setMelodyEnvelope(
		uint32_t sampleFreq,		    // Hz
		float attack_time,				// sec
		float decay_time,				// sec
		parameter_type sustain_level,	// level (0 <= x <= 1)
		float release_time				// sec
	)
	{
		setMelodyEnvelope(sampleFreq, {}, attack_time, 0, decay_time, sustain_level, 0, release_time, std::numeric_limits<parameter_type>::epsilon());
	}
	void setMelodyEnvelope(
		uint32_t sampleFreq,			// Hz
		Curve curve,					
		float attack_time,				// sec
		float hold_time,				// sec
		float decay_time,				// sec
		parameter_type sustain_level,	// level
		float fade_slope,				// Linear : level/sec, Exp : dBFS/sec
		float release_time,				// sec
		parameter_type cutoff_level		// level
	)
	{
		mModel = Model::Melody;
		mCurve = curve;

		// 各種パラメータの範囲調整 & サンプル単位に変換 
		mAttackTime   = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * attack_time));
		mHoldTime     = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * hold_time));
		mDecayTime    = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * decay_time));
		mReleaseTime  = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * release_time));
		mSustainLevel = std::clamp<parameter_type>(sustain_level, 0, 1);
		mFadeSlope    = std::min<parameter_type>(fade_slope, 0) / sampleFreq; // 減衰率なので負の値
		mCutOffLevel  = std::clamp<parameter_type>(cutoff_level, 0, 1);
	}
	// エンベロープ形状パラメータを指定します (ドラム用)
	void setDrumEnvelope(
		uint32_t sampleFreq,			// Hz
		Curve curve,
		float attack_time,				// sec
		float hold_time,				// sec
		float decay_time,				// sec
		parameter_type cutoff_level		// level
	)
	{
		mModel = Model::Drum;
		mCurve = curve;

		// 各種パラメータの範囲調整 & サンプル単位に変換 
		mAttackTime = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * attack_time));
		mHoldTime = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * hold_time));
		mDecayTime = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * decay_time));
		mReleaseTime = 0; // unused
		mSustainLevel = 0; // unused
		mFadeSlope = 0; // unused
		mCutOffLevel = std::clamp<parameter_type>(cutoff_level, 0, 1);
	}

	// ノートオン (Attackへ遷移)
	void noteOn()
	{
		switchToAttack();
	}

	// ノートオフ (Releaseへ遷移)
	void noteOff()
	{
		if(mModel != Model::Drum) {
			switchToRelease(envelope());
		}
	}

	// エンベロープを計算します
	parameter_type envelope()const
	{
		auto easing = [this](uint64_t max)->parameter_type { 
			const auto p = parameter_type(mTime) / parameter_type(max);

			switch(mCurve) {
			case Shape::Linear: 
				// 線形補間
				return mBeginLevel * (1-p) + mEndLevel * p;
			case Shape::Exp: {
				// Exp : RL回路ステップ応答型
				const parameter_type n = mCurve.exp_param_n;
				const parameter_type level_at_n = mCurve.exp_param_level_at_n;
				auto v = (1 - std::exp(- p * n)) / level_at_n;
				return mBeginLevel + (mEndLevel - mBeginLevel) * v;
			}
			}
			Assertion::unreachable("invalid shape");
		};
		auto easing_slope = [this]()->parameter_type { 
			switch(mCurve) {
			case Shape::Linear: 
				// Linear : level/sec
				return std::max<parameter_type>(0, mBeginLevel + mFadeSlope * mTime);
			case Shape::Exp: {
				// Exp : dBFS/sec
				return static_cast<parameter_type>(mBeginLevel * std::pow(10, mFadeSlope/20 * mTime));
			}
			}
			Assertion::unreachable("invalid shape");
		};

		switch (mState) {
		case EnvelopeState::Attack:
			return easing(mAttackTime);
		case EnvelopeState::Hold:
			return easing(mHoldTime);
		case EnvelopeState::Decay:
			return easing(mDecayTime);
		case EnvelopeState::Release: 
			return easing(mReleaseTime);
		case EnvelopeState::Fade:
			return easing_slope();
		case EnvelopeState::Free:
			return 0;// 止音中
		}
		Assertion::unreachable("invalid state");
	}

	// エンベロープを計算し、状態を更新します
	parameter_type update()
	{
		auto v = envelope();

		++mTime;

		switch (mState) {
		case EnvelopeState::Attack:
			// アタック中
			if (mTime < mAttackTime) {
				break;
			} else {
				mTime -= mAttackTime;
				switchToHold(); // アタック → ホールド
				[[fallthrough]];
			}
		case EnvelopeState::Hold:
			// ホールド中
			if (mTime < mHoldTime) {
				break;
			} else {
				mTime -= mHoldTime;
				switchToDecay(); // ホールド → ディケイ
				[[fallthrough]];
			}
		case EnvelopeState::Decay:
			// ディケイ中
			if (mTime < mDecayTime) {
				break;
			}
			else if(mModel == Model::Drum) {
				if(v <= mCutOffLevel) {
					// ドラム : 音量が規定値を下回った場合 : ノートオフを待たずに止音
					switchToFree();
				}
				else {
					mTime -= mDecayTime;
					switchToFree(); // ディケイ → リリース
				}
			}
			else {
				mTime -= mDecayTime;
				switchToFade(); // ディケイ → フェード
			}
			[[fallthrough]];
		case EnvelopeState::Fade:
			// フェード中
			if (v <= mCutOffLevel) {
				// 音量が規定値を下回った場合 : ノートオフを待たずに止音
				switchToFree();
			}
			break;
		case EnvelopeState::Release: 
			// リリース中
			if(mTime >= mReleaseTime) {
				// リリース時間終了 => 止音
				switchToFree();
			} else if(v <= mCutOffLevel) {
				// 音量が規定値を下回った場合 : ノートオフを待たずに止音
				switchToFree();
			}
			break;
		case EnvelopeState::Free:
			// 止音中
			// do-nothing
			break;
		}
		return v;
	}

	// エンベロープジェネレータが動作中か否か
	bool isBusy()const noexcept 
	{
		return mState != EnvelopeState::Free;
	}

	// エンベロープジェネレータ ステート
	EnvelopeState state()const noexcept
	{
		return mState;
	}

protected:
	void switchToAttack() 
	{
		mState = EnvelopeState::Attack;
		mBeginLevel = 0;
		mEndLevel = 1;

		mTime = 0;
	}
	void switchToHold() 
	{
		mState = EnvelopeState::Hold;
		mBeginLevel = 1;
		mEndLevel = 1;
	}
	void switchToDecay() 
	{
		mState = EnvelopeState::Decay;
		mBeginLevel = 1;
		mEndLevel = mSustainLevel;
	}
	void switchToFade() 
	{
		mState = EnvelopeState::Fade;
		mBeginLevel = mSustainLevel;
		mEndLevel = 0;
	}
	void switchToRelease(parameter_type current_level) 
	{
		if(mState == EnvelopeState::Free) return; // そもそも止音中のため、Releaseに遷移する必要は無い

		mState = EnvelopeState::Release;
		mBeginLevel = current_level;
		mEndLevel = 0;

		mTime = 0;
	}
	void switchToFree() 
	{
		mState = EnvelopeState::Free;
		mBeginLevel = 0;
		mEndLevel = 0;
	}

private:	
	Model mModel = Model::Melody;
	EnvelopeState mState;			// 現在の状態
	uint64_t mTime;					// 現在の時刻(ノートオン/ノートオフからの)
	Curve mCurve;
	parameter_type mBeginLevel;		
	parameter_type mEndLevel;		

	uint64_t mAttackTime;			// sample
	uint64_t mHoldTime;				// sample
	uint64_t mDecayTime;			// sample
	parameter_type mSustainLevel;	// level (0 <= x <= 1)
	parameter_type mFadeSlope;		// Linear : level/sample, Exp : dBFS/sample
	uint64_t mReleaseTime;			// sample
	parameter_type mCutOffLevel;	// level (0 <= x <= 1)
};


}