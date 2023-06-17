#pragma once

#include <LSP/Synth/Base.hpp>
#include <LSP/Synth/WaveTable.hpp>
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

	struct Digest {
		float freq = 0; // 基本周波数
		float envelope = 0; // エンベロープジェネレータ出力
		LSP::Filter::EnvelopeState state = LSP::Filter::EnvelopeState::Free; // エンベロープジェネレータ ステート
	};

public:
	Voice(uint32_t sampleFreq, const EnvelopeGenerator& eg, uint32_t noteNo, float pitchBend, float volume, bool hold);
	virtual ~Voice();

	virtual float update() = 0;

	Digest digest()const noexcept;

	uint32_t noteNo()const noexcept;
	void noteOff()noexcept;
	void noteCut()noexcept;

	void setHold(bool hold)noexcept;

	std::optional<float> pan()const noexcept;
	void setPan(float pan)noexcept;

	void setPitchBend(float pitchBend)noexcept;
	const EnvelopeGenerator& envolopeGenerator()const noexcept;


protected:

	void updateFreq()noexcept;

protected:
	const uint32_t mSampleFreq;
	EnvelopeGenerator mEG;
	uint32_t mNoteNo;
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
	using WaveTableGenerator = LSP::Generator::WaveTableGenerator<float>;


public:
	WaveTableVoice(uint32_t sampleFreq, const WaveTableGenerator& wg, const EnvelopeGenerator& eg, uint32_t noteNo, float pitchBend, float volume, bool hold)
		: Voice(sampleFreq, eg, noteNo, pitchBend, volume, hold)
		, mWG(wg)
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