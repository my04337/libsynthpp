#pragma once

#include <LSP/MIDI/Synthesizer/Tone.hpp>

#include <LSP/Generator/FunctionGenerator.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>
#include <LSP/Filter/BiquadraticFilter.hpp>
#include <LSP/Filter/Requantizer.hpp>

namespace LSP::MIDI::Synthesizer
{

// シンプルなトーン : ファンクションジェネレータ & EG & LPFの基本的な構成
template<
	typename sample_type,
	typename parameter_type = sample_type,
	class = std::enable_if_t<
	is_sample_type_v<sample_type> && is_floating_point_sample_type_v<parameter_type>
	>
>
class SimpleSinTone
	: public Tone<sample_type>
{
public:
	using function_generator_type = Generator::FunctionGenerator<sample_type, parameter_type>;
	using envelope_generator_type = Filter::EnvelopeGenerator<parameter_type>;
	using bqf_type = Filter::BiquadraticFilter<sample_type, parameter_type>;

	SimpleSinTone(function_generator_type osc, bqf_type lpf, envelope_generator_type eg, parameter_type vol = 1)
		: mOsc(osc)
		, mLPF(lpf)
		, mEG(eg)
		, mToneVol(vol)
	{
		// MEMO EG、LPFは共にパラメータの計算コストが高いため、計算済みのものを受け取る

		lsp_assert(!mEG.isBusy());
		mEG.noteOn();
	}
	
	// ノートオフ
	virtual void noteOff()override
	{
		mEG.noteOff();
	}

	// 音生成
	virtual sample_type update() override
	{
		constexpr auto to_sample_type = Filter::Requantizer<sample_type>();
		constexpr auto to_param_type = Filter::Requantizer<parameter_type>();

		sample_type v = mOsc.update();
		v = mLPF.update(v);
		v = to_sample_type(to_param_type(v) * mToneVol * mEG.update());
		return v;
	}

	// 出音中か否か
	virtual bool isBusy()const noexcept { return mEG.isBusy(); }
	
private:
	function_generator_type mOsc;
	bqf_type mLPF;
	envelope_generator_type mEG;
	parameter_type mToneVol;
};

}