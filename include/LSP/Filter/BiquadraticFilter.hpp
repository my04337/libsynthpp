#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Math.hpp>
#include <LSP/Filter/Requantizer.hpp>

namespace LSP::Filter {

// 双二次フィルタクラス
//   参考URL : http://ufcpp.net/study/sp/digital_filter/biquad/
//             http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
template<
	typename sample_type,
	typename parameter_type = sample_type,
	class = std::enable_if_t<
		is_sample_type_v<sample_type> && is_floating_point_sample_type_v<parameter_type>
	>
>
class BiquadraticFilter
{
public:
	BiquadraticFilter()
	{
		clear(); 
	}

	// 初期化
	void clear() noexcept
	{
		resetParam();
		resetState();
	}

	// フィルタパラメータのみ初期化
	void resetParam() noexcept
	{
		// 入力をそのまま出力するパラメータ
		a0 = 1; b0 = 1;
		a1 = 0; a2 = 0;
		b1 = 0; b2 = 0;
	}

	// 内部ステートのみ初期化
	void resetState() noexcept
	{
		x[0] = 0; x[1] = 0; 
		y[0] = 0; y[1] = 0;
		idx1 = 0; idx2 = 1;
	}


	// 出力更新
	sample_type update(float_t x0) noexcept
	{
		using conv = Requantizer<parameter_type, sample_type>;

		const auto x1 = x[idx1], x2 = x[idx2];
		const auto y1 = y[idx1], y2 = y[idx2];
		
		// MEMO 計算式の由来はAudio-EQ-Cookbook Eq4式を参照のこと
		parameter_type y0 = (b0*x0 +b1*x1 +b2*x2 -a1*y1 -a2*y2)/a0;

		// 内部ステート更新
		x[idx2] = x0;
		y[idx2] = y0;
		idx1 ^= 0x01;
		idx2 ^= 0x01;

		return conv()(y0);
	}


	// パラメータ設定 : ローパスフィルタ
	void setLopassParam(parameter_type sampleFreq, parameter_type cutOffFreq, parameter_type Q)
	{
		const parameter_type w0    = 2 * PI<parameter_type> * cutOffFreq / sampleFreq;
		const parameter_type sinw0 = sin(w0);
		const parameter_type cosw0 = cos(w0);
		const parameter_type alpha = sinw0 / (2 * Q);

		b0 = (1 - cosw0) / 2;
		b1 = 1 - cosw0;
		b2 = b0;
		a0 = 1 + alpha;
		a1 = -2 * cosw0;
		a2 = 1 - alpha;
	}

	// パラメータ設定 : ハイパスフィルタ
	void setHighpassParam(parameter_type sampleFreq, parameter_type cutOffFreq, parameter_type Q)
	{
		const parameter_type w0    = 2 * PI<parameter_type> * cutOffFreq / sampleFreq;
		const parameter_type sinw0 = sin(w0);
		const parameter_type cosw0 = cos(w0);
		const parameter_type alpha = sinw0 / (2 * Q);

		b0 = (1 + cosw0) / 2;
		b1 = -(1 + cosw0);
		b2 = b0;
		a0 = 1 + alpha;
		a1 = -2 * cosw0;
		a2 = 1 - alpha;
	}

	// パラメータ設定 : バンドパス1
	void setBandpass1Param(parameter_type sampleFreq, parameter_type centerFreq, parameter_type BW)
	{
		const parameter_type w0    = 2 * PI<parameter_type> * centerFreq / sampleFreq;
		const parameter_type sinw0 = sin(w0);
		const parameter_type cosw0 = cos(w0);
		const parameter_type alpha = 2 * sinw0 / BW;

		b0 = BW * alpha;
		b1 = 0;
		b2 = -BW * alpha;
		a0 = 1 + alpha;
		a1 = -2 * cosw0;
		a2 = 1 - alpha;
	}

	// パラメータ設定 : バンドパス2
	void setBandpass2Param(parameter_type sampleFreq, parameter_type centerFreq, parameter_type BW)
	{
		const parameter_type w0    = 2 * PI<parameter_type> * centerFreq / sampleFreq;
		const parameter_type sinw0 = sin(w0);
		const parameter_type cosw0 = cos(w0);
		const parameter_type log2  = log((parameter_type)2);
		const parameter_type alpha = sinw0 * sinh(log2 * BW * w0 / sinw0);

		b0 = alpha;
		b1 = 0;
		b2 = -alpha;
		a0 = 1 + alpha;
		a1 = -2 * cosw0;
		a2 = 1 - alpha;
	}

	// パラメータ設定 : バンドストップ
	void setBandstopParam(parameter_type sampleFreq, parameter_type centerFreq, parameter_type BW)
	{
		const parameter_type w0    = 2 * PI<parameter_type> * centerFreq / sampleFreq;
		const parameter_type sinw0 = sin(w0);
		const parameter_type cosw0 = cos(w0);
		const parameter_type log2  = log((parameter_type)2);
		const parameter_type alpha = sinw0 * sinh(log2 * BW * w0 / sinw0);

		b0 = 1;
		b1 = -2 * cosw0;
		b2 = 1;
		a0 = 1 + alpha;
		a1 = -2 * cosw0;
		a2 = 1 - alpha;
	}

	// パラメータ設定 : オールパス
	void setAllpassParam(parameter_type sampleFreq, parameter_type cutOffFreq, parameter_type BW)
	{
		const parameter_type w0    = 2 * PI<parameter_type> * cutOffFreq / sampleFreq;
		const parameter_type sinw0 = sin(w0);
		const parameter_type cosw0 = cos(w0);
		const parameter_type log2  = log((parameter_type)2);
		const parameter_type alpha = sinw0 * sinh(log2 * BW * w0 / sinw0);

		b0 = 1 - alpha;
		b1 = -2 * cosw0;
		b2 = 1 + alpha;
		a0 = 1 + alpha;
		a1 = -2 * cosw0;
		a2 = 1 - alpha;
	}

	// パラメータ設定 : ピーキング
	void setPeakingParam(parameter_type sampleFreq, parameter_type centerFreq, parameter_type BW, parameter_type gain)
	{
		const parameter_type w0    = 2 * PI<parameter_type> * centerFreq / sampleFreq;
		const parameter_type sinw0 = sin(w0);
		const parameter_type cosw0 = cos(w0);
		const parameter_type log2  = log((parameter_type)2);
		const parameter_type alpha = sinw0 * sinh(log2 * BW * w0 / sinw0);
		const parameter_type A     = sqrt(pow((parameter_type)10, gain / 20));

		b0 = 1 + alpha * A;
		b1 = -2 * cosw0;
		b2 = 1 - alpha * A;
		a0 = 1 + alpha / A;
		a1 = -2 * cosw0;
		a2 = 1 - alpha / A;
	}

	// パラメータ設定 : ローシェルフ
	void setLoshelfParam(parameter_type sampleFreq, parameter_type cutOffFreq, parameter_type S, parameter_type gain)
	{
		const parameter_type w0    = 2 * PI<parameter_type> * cutOffFreq / sampleFreq;
		const parameter_type sinw0 = sin(w0);
		const parameter_type cosw0 = cos(w0);
		const parameter_type A     = sqrt(pow((parameter_type)10, gain / 20));
		const parameter_type alpha = sinw0 / 2 * sqrt((A + 1 / A) * (1 / S - 1) + 2);
		const parameter_type sqrtA = sqrt(A);

		b0 = A * ((A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha);
		b1 = 2 * A * ((A - 1) - (A + 1) * cosw0);
		b2 = A * ((A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha);
		a0 = (A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha;
		a1 = -2 * ((A - 1) + (A + 1) * cosw0);
		a2 = (A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha;
	}

	// パラメータ設定 : ハイシェルフ
	void setHighshelfParam(parameter_type sampleFreq, parameter_type cutOffFreq, parameter_type S, parameter_type gain)
	{
		const parameter_type w0    = 2 * PI<parameter_type> * cutOffFreq / sampleFreq;
		const parameter_type sinw0 = sin(w0);
		const parameter_type cosw0 = cos(w0);
		const parameter_type A     = sqrt(pow((parameter_type)10, gain / 20));
		const parameter_type alpha = sinw0 / 2 * sqrt((A + 1 / A) * (1 / S - 1) + 2);
		const parameter_type sqrtA = sqrt(A);

		b0 = A * ((A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha);
		b1 = -2 * A * ((A - 1) + (A + 1) * cosw0);
		b2 = A * ((A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha);
		a0 = (A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha;
		a1 = 2 * ((A - 1) - (A + 1) * cosw0);
		a2 = (A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha;
	}

private:
	parameter_type x[2],y[2];
	int idx1, idx2;
	parameter_type a0,a1,a2,b0,b1,b2;
};

}