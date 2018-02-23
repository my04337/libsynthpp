#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP {

// 双二次フィルタクラス
//   参考URL : http://ufcpp.net/study/sp/digital_filter/biquad/
class BiquadraticFilter
{
public:
	BiquadraticFilter();

	// 初期化
	void clear();

	// 内部ステートのみ初期化
	void resetState();

	// フィルタパラメータのみ初期化
	void resetParam();

	// 出力更新
	float_t update(float_t x0);


	// パラメータ設定 : ローパスフィルタ
	void setLopassParam(float_t sampleFreq, float_t cutOffFreq, float_t Q);

	// パラメータ設定 : ハイパスフィルタ
	void setHighpassParam(float_t sampleFreq, float_t cutOffFreq, float_t Q);

	// パラメータ設定 : バンドパス1
	void setBandpass1Param(float_t sampleFreq, float_t centerFreq, float_t BW);

	// パラメータ設定 : バンドパス2
	void setBandpass2Param(float_t sampleFreq, float_t centerFreq, float_t BW);

	// パラメータ設定 : バンドストップ
	void setBandstopParam(float_t sampleFreq, float_t centerFreq, float_t BW);

	// パラメータ設定 : オールパス
	void setAllpassParam(float_t sampleFreq, float_t cutOffFreq, float_t BW);

	// パラメータ設定 : ピーキング
	void setPeakingParam(float_t sampleFreq, float_t centerFreq, float_t BW, float_t gain);

	// パラメータ設定 : ローシェルフ
	void setLoshelfParam(float_t sampleFreq, float_t cutOffFreq, float_t S, float_t gain);

	// パラメータ設定 : ハイシェルフ
	void setHighshelfParam(float_t sampleFreq, float_t cutOffFreq, float_t S, float_t gain);

private:
	float_t x[2],y[2];
	int idx1, idx2;
	float_t a0,a1,a2,b0,b1,b2;
};

}