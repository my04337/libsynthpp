#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>

namespace LSP::Filter
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

	// エンベロープ形状パラメータを指定します
	void setParam(
		parameter_type sampleFreq,      // Hz
		parameter_type attack_time,		// sec
		parameter_type decay_time,		// sec
		parameter_type sustain_level,	// level (0 <= x <= 1)
		parameter_type release_time		// sec
	)
	{
		setParam(sampleFreq, {}, attack_time, 0, decay_time, sustain_level, 0, release_time);
	}
	void setParam(
		parameter_type sampleFreq,		// Hz
		Curve curve,					
		parameter_type attack_time,		// sec
		parameter_type hold_time,		// sec
		parameter_type decay_time,		// sec
		parameter_type sustain_level,	// level
		parameter_type fade_slope,		// Linear : level/sec, Exp : dBFS/sec
		parameter_type release_time		// sec)
	)
	{
		mSampleFreq = sampleFreq;
		// 各種パラメータの範囲調整 & サンプル単位に変換 
		mAttackTime   = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * attack_time));
		mHoldTime     = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * hold_time));
		mDecayTime    = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * decay_time));
		mReleaseTime  = std::max<uint64_t>(0, static_cast<uint64_t>(sampleFreq * release_time));

		mSustainLevel = std::clamp<parameter_type>(sustain_level, 0, 1);
		mFadeSlope    = std::min<parameter_type>(fade_slope, 0) / sampleFreq; // 減衰率なので負の値

		mCurve = curve;
	}

	// サンプリング周波数
	parameter_type sampleFreq()
	{
		return mSampleFreq;
	}

	// ノートオン (Attackへ遷移)
	void noteOn()
	{
		switchToAttack();
	}

	// ノートオフ (Releaseへ遷移)
	void noteOff()
	{
		switchToRelease(envelope());
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
			lsp_assert_desc(false, "invalid shape");
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
			lsp_assert_desc(false, "invalid shape");
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
		lsp_assert_desc(false, "invalid state");
	}

	// エンベロープを計算し、状態を更新します
	parameter_type update()
	{
		constexpr auto epsilon = std::numeric_limits<parameter_type>::epsilon();

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
			} else {
				mTime -= mDecayTime;
				switchToFade(); // ディケイ → フェード
				[[fallthrough]];
			}
		case EnvelopeState::Fade:
			// フェード中
			if (v < epsilon) {
				// エンベロープ0 : ノートオフを待たずに止音
				switchToFree();
			}
			break;
		case EnvelopeState::Release: 
			// リリース中
			if(mTime < mReleaseTime) {
				break;
			} else {
				// リリース時間終了 => 止音
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
	parameter_type mSampleFreq=0;	// サンプリング周波数
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
};


}