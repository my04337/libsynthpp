#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Id.hpp>
#include <LSP/Base/Sample.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>
#include <LSP/Generator/WaveTableGenerator.hpp>

namespace LSP::Synth
{
// ボイス識別番号
struct _voice_id_tag {};
using VoiceId = issuable_id_base_t<_voice_id_tag>;


// ボイス(あるチャネルの1音) - 基底クラス
class Voice
	: non_copy_move
{
public:
	using EnvelopeGenerator = LSP::Filter::EnvelopeGenerator<float>;
	using EnvelopeState = LSP::Filter::EnvelopeState;

	struct Info {
		float freq = 0; // 基本周波数
		float envelope = 0; // エンベロープジェネレータ出力
		EnvelopeState state = EnvelopeState::Free; // エンベロープジェネレータ ステート
	};

public:
	Voice(size_t sampleFreq, const EnvelopeGenerator& eg, uint32_t noteNo, float pitchBend, float volume);
	virtual ~Voice();

	virtual float update() = 0;

	Info info()const noexcept;

	void setPitchBend(float pitchBend)noexcept;
	EnvelopeGenerator& envolopeGenerator()noexcept;
	const EnvelopeGenerator& envolopeGenerator()const noexcept;


protected:

	void updateFreq()noexcept;

protected:
	const size_t mSampleFreq;
	EnvelopeGenerator mEG;
	uint32_t mNoteNo;
	float mPitchBend;
	float mCalculatedFreq = 0;
	float mVolume;
};


// ---


// 波形メモリ ボイス実装
class WaveTableVoice
	: public Voice
{
public:
	using WaveTableGenerator = LSP::Generator::WaveTableGenerator<float>;


public:
	WaveTableVoice(size_t sampleFreq, SignalView<float> waveTable, const EnvelopeGenerator& eg, uint32_t noteNo, float pitchBend, float volume)
		: Voice(sampleFreq, eg, noteNo, pitchBend, volume)
		, mWG(waveTable)
	{}


	virtual ~WaveTableVoice() {}

	virtual float update()override
	{
		auto v = mWG.update(mSampleFreq, mCalculatedFreq);
		v *= mEG.update();
		v *= mVolume;
		return v;
	}

private:
	WaveTableGenerator mWG;
};

}