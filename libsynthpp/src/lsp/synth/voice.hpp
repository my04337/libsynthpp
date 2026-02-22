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
	using MelodyEG = dsp::MelodyEnvelopeGenerator<float>;
	using DrumEG = dsp::DrumEnvelopeGenerator<float>;
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

	// ダンパーペダル(CC:64)によるホールド状態を設定します
	// Hold中にnoteOffを受信した場合、リリースを保留しHold解除時にリリースします
	void setHold(bool hold)noexcept;

	// ソステヌート(CC:66)状態を設定します
	// ソステヌートはHoldと異なり、ペダルを踏んだ瞬間に打鍵中のボイスのみを保持対象とします
	// ソステヌート対象のボイスがnoteOffを受信した場合、リリースを保留しソステヌート解除時にリリースします
	// ※ HoldとSostenutoは独立して動作し、いずれか一方でも有効であればリリースは保留されます
	void setSostenuto(bool sostenuto)noexcept;

	// キーが現在押下中(noteOnされてからnoteOffもリリースもされていない)かどうかを返します
	// ソステヌートペダルON時の対象ボイス判定に使用します
	bool isNoteOn()const noexcept;

	std::optional<float> pan()const noexcept;
	void setPan(float pan)noexcept;

	void setPitchBend(float pitchBend)noexcept;
	void setPolyPressure(float pressure)noexcept;

	// ノート番号オフセットを設定します (楽器定義による移調やドラムピッチ調整用)
	// このオフセットは周波数計算にのみ使用され、noteNo()によるマッチングには影響しません
	void setNoteOffset(float offset)noexcept;

	// 実際に発音しているノート番号 (noteNo + offset) を返します
	// フィルタ計算等、実際の発音周波数に基づく処理に使用します
	float soundingNoteNo()const noexcept;

	// EG の状態取得 (派生クラスで実装)
	virtual float envelope()const noexcept = 0;
	virtual EnvelopeState envelopeState()const noexcept = 0;
	virtual bool isBusy()const noexcept = 0;

	// ローパスフィルタのパラメータを設定します
	// CC#74 (Brightness) でカットオフ周波数、CC#71 (Resonance) でQ値を制御します
	void setFilter(float cutoffFreq, float Q)noexcept;

	// ベースリリースタイム(楽器定義から決まる値)を設定します
	void setBaseReleaseTime(float timeSec)noexcept;
	// リリースタイムのスケーリング係数を更新し、EGに反映します
	// CC:72やNRPN(1,102)の変更時に呼び出されます
	virtual void setReleaseTimeScale(float scale)noexcept;


protected:
	void updateFreq()noexcept;

	// 派生クラスで実装するEG操作
	virtual void onNoteOff()noexcept = 0;
	virtual void onNoteCut()noexcept = 0;

protected:
	const uint32_t mSampleFreq;
	BiquadraticFilter mFilter; // ローパスフィルタ (CC#74: cutoff, CC#71: Q)
	float mNoteNo;
	float mNoteOffset = 0; // 周波数計算用のノート番号オフセット (楽器定義による移調等)
	bool mPendingNoteOff = false; // Hold/Sostenutoによりリリースが保留されている場合にtrue
	bool mHold = false;            // CC:64 ダンパーペダル(チャネル全体)
	bool mSostenuto = false;       // CC:66 ソステヌート(ペダルON時に打鍵中だったボイスのみ)
	float mPitchBend;
	float mCalculatedFreq = 0;
	float mVolume;
	float mPolyPressure = 1.0f; // ポリフォニックキープレッシャー [0.0, 1.0]
	std::optional<float> mPan; // ドラムなど、ボイス毎にパンが指定される場合のヒント
	float mBaseReleaseTimeSec = 0; // 楽器定義から決まるベースリリースタイム(秒)
};


// 波形メモリ ボイス実装 (メロディパート用)
class MelodyWaveTableVoice
	: public Voice
{
public:
	using WaveTableGenerator = dsp::WaveTableGenerator<float>;

public:
	MelodyWaveTableVoice(uint32_t sampleFreq, WaveTableGenerator&& wg, float noteNo, float pitchBend, float volume, bool hold)
		: Voice(sampleFreq, noteNo, pitchBend, volume, hold)
		, mWG(std::move(wg))
	{}

	virtual ~MelodyWaveTableVoice() {}

	virtual float update()override
	{
		auto v = mWG.update(static_cast<float>(mSampleFreq), mCalculatedFreq);
		v = mFilter.update(v);
		v *= mEG.update();
		v *= mVolume;
		v *= mPolyPressure;
		return v;
	}

	virtual float envelope()const noexcept override { return mEG.envelope(); }
	virtual EnvelopeState envelopeState()const noexcept override { return mEG.state(); }
	virtual bool isBusy()const noexcept override { return mEG.isBusy(); }
	virtual void setReleaseTimeScale(float scale)noexcept override
	{
		mEG.setReleaseTime(static_cast<float>(mSampleFreq), std::max(0.001f, mBaseReleaseTimeSec * scale));
	}

	MelodyEG& envelopeGenerator() noexcept { return mEG; }

protected:
	virtual void onNoteOff()noexcept override { mEG.noteOff(); }
	virtual void onNoteCut()noexcept override { mEG.reset(); }

private:
	WaveTableGenerator mWG;
	MelodyEG mEG;
};

// 波形メモリ ボイス実装 (ドラムパート用)
class DrumWaveTableVoice
	: public Voice
{
public:
	using WaveTableGenerator = dsp::WaveTableGenerator<float>;

public:
	DrumWaveTableVoice(uint32_t sampleFreq, WaveTableGenerator&& wg, float noteNo, float pitchBend, float volume, bool hold)
		: Voice(sampleFreq, noteNo, pitchBend, volume, hold)
		, mWG(std::move(wg))
	{}

	virtual ~DrumWaveTableVoice() {}

	virtual float update()override
	{
		auto v = mWG.update(static_cast<float>(mSampleFreq), mCalculatedFreq);
		v = mFilter.update(v);
		v *= mEG.update();
		v *= mVolume;
		v *= mPolyPressure;
		return v;
	}

	virtual float envelope()const noexcept override { return mEG.envelope(); }
	virtual EnvelopeState envelopeState()const noexcept override { return mEG.state(); }
	virtual bool isBusy()const noexcept override { return mEG.isBusy(); }

	DrumEG& envelopeGenerator() noexcept { return mEG; }

protected:
	virtual void onNoteOff()noexcept override { mEG.noteOff(); }
	virtual void onNoteCut()noexcept override { mEG.reset(); }

private:
	WaveTableGenerator mWG;
	DrumEG mEG;
};

}