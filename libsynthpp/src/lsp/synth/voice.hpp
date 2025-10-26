#pragma once

#include <lsp/core/core.hpp>

#include <lsp/dsp/envelope_generator.hpp>
#include <lsp/dsp/biquadratic_filter.hpp>
#include <lsp/dsp/wave_table_generator.hpp>

namespace lsp::synth
{
// ボイス識別番号
struct _voice_id_tag {};
using VoiceId = issuable_id_base_t<_voice_id_tag>;


// ボイス(あるチャネルの1音) - 基底クラス
class Voice
	: non_copy_move
{
public:
	using EnvelopeGenerator = dsp::EnvelopeGenerator<float>;
	using EnvelopeState = dsp::EnvelopeState;
	using BiquadraticFilter = dsp::BiquadraticFilter<float>;

	struct Digest {
		float freq = 0; // 基本周波数
		float envelope = 0; // エンベロープジェネレータ出力
		dsp::EnvelopeState state = dsp::EnvelopeState::Free; // エンベロープジェネレータ ステート
	};

public:
	Voice(uint32_t sampleFreq, float noteNo, float pitchBend, float volume, bool hold);
	virtual ~Voice();

	virtual float update() = 0;

	Digest digest()const noexcept;

	float noteNo()const noexcept;
	void noteOff()noexcept;
	void noteCut()noexcept;

	void setHold(bool hold)noexcept;

	std::optional<float> pan()const noexcept;
	void setPan(float pan)noexcept;

	void setPitchBend(float pitchBend)noexcept;
	EnvelopeGenerator& envolopeGenerator() noexcept;

	void setCutOff(float freqRate, float cutOffGain);
	void setResonance(float freqRate, float overtoneGain);


protected:
	void updateFreq()noexcept;

protected:
	const uint32_t mSampleFreq;
	EnvelopeGenerator mEG;
	BiquadraticFilter mCutOffFilter;
	BiquadraticFilter mResonanceFilter;
	float mNoteNo;
	bool mPendingNoteOff = false;
	bool mHold = false;
	float mPitchBend;
	float mCalculatedFreq = 0;
	float mVolume;
	std::optional<float> mPan; // ドラムなど、ボイス毎にパンが指定される場合のヒント
};


// 波形メモリ ボイス実装
class WaveTableVoice
	: public Voice
{
public:
	using WaveTableGenerator = dsp::WaveTableGenerator<float>;


public:
	WaveTableVoice(uint32_t sampleFreq, WaveTableGenerator&& wg, float noteNo, float pitchBend, float volume, bool hold)
		: Voice(sampleFreq, noteNo, pitchBend, volume, hold)
		, mWG(std::move(wg))
	{}


	virtual ~WaveTableVoice() {}

	virtual float update()override
	{
		auto v = mWG.update(mSampleFreq, mCalculatedFreq);
		v = mCutOffFilter.update(v);
		v = mResonanceFilter.update(v);
		v *= mEG.update();
		v *= mVolume;
		return v;
	}

private:
	WaveTableGenerator mWG;
};

}