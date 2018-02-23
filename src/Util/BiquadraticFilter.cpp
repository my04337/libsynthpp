#include <LSP/Util/BiquadraticFilter.hpp>

using namespace LSP;


BiquadraticFilter::BiquadraticFilter()
{
	clear();
}

void BiquadraticFilter::clear()
{
	resetState();
	resetParam();
}

void BiquadraticFilter::resetState()
{
	x[0] = y[0] = x[1] = y[1] = 0;
	idx1 = 0;
	idx2 = 1;
}

void BiquadraticFilter::resetParam()
{
	a0 = b0 = 1;
	a1 = a2 = b1 = b2 = 0;
}

float_t BiquadraticFilter::update(float_t x0)
{
	float_t y0 = (b0*x0 +b1*x[idx1] +b2*x[idx2] -a1*y[idx1] -a2*y[idx2])/a0;

	x[idx2] = x0;
	y[idx2] = y0;
	idx1 ^= 0x01;
	idx2 ^= 0x01;
	return y0;
}

void BiquadraticFilter::setLopassParam(float_t sampleFreq, float_t cutOffFreq, float_t Q) 
{
	const float_t w0    = 2 * PI * cutOffFreq / sampleFreq;
	const float_t sinw0 = sin(w0);
	const float_t cosw0 = cos(w0);
	const float_t alpha = 2 * sinw0 / Q;

	b0 = (1 - cosw0) / 2;
	b1 = 1 - cosw0;
	b2 = b0;
	a0 = 1 + alpha;
	a1 = -2 * cosw0;
	a2 = 1 - alpha;
}

void BiquadraticFilter::setHighpassParam(float_t sampleFreq,float_t cutOffFreq,float_t Q) 
{
	const float_t w0    = 2 * PI * cutOffFreq / sampleFreq;
	const float_t sinw0 = sin(w0);
	const float_t cosw0 = cos(w0);
	const float_t alpha = 2 * sinw0 / Q;

	b0 = (1 + cosw0) / 2;
	b1 = -(1 + cosw0);
	b2 = b0;
	a0 = 1 + alpha;
	a1 = -2 * cosw0;
	a2 = 1 - alpha;
}

void BiquadraticFilter::setBandpass1Param(float_t sampleFreq, float_t centerFreq, float_t BW) 
{
	const float_t w0    = 2 * PI * centerFreq / sampleFreq;
	const float_t sinw0 = sin(w0);
	const float_t cosw0 = cos(w0);
	const float_t alpha = 2 * sinw0 / BW;

	b0 = BW * alpha;
	b1 = 0;
	b2 = -BW * alpha;
	a0 = 1 + alpha;
	a1 = -2 * cosw0;
	a2 = 1 - alpha;
}

void BiquadraticFilter::setBandpass2Param(float_t sampleFreq, float_t centerFreq, float_t BW) 
{
	const float_t w0    = 2 * PI * centerFreq / sampleFreq;
	const float_t sinw0 = sin(w0);
	const float_t cosw0 = cos(w0);
	const float_t alpha = sinw0 * sinh(log(2.0f) * BW * w0 / sinw0);

	b0 = alpha;
	b1 = 0;
	b2 = -alpha;
	a0 = 1 + alpha;
	a1 = -2 * cosw0;
	a2 = 1 - alpha;
}

void BiquadraticFilter::setBandstopParam(float_t sampleFreq,float_t centerFreq,float_t BW) 
{
	const float_t w0    = 2 * PI * centerFreq / sampleFreq;
	const float_t sinw0 = sin(w0);
	const float_t cosw0 = cos(w0);
	const float_t alpha = sinw0 * sinh(log(2.0f) * BW * w0 / sinw0);

	b0 = 1;
	b1 = -2 * cosw0;
	b2 = 1;
	a0 = 1 + alpha;
	a1 = -2 * cosw0;
	a2 = 1 - alpha;
}

void BiquadraticFilter::setAllpassParam(float_t sampleFreq,float_t cutOffFreq,float_t BW) 
{
	const float_t w0    = 2 * PI * cutOffFreq / sampleFreq;
	const float_t sinw0 = sin(w0);
	const float_t cosw0 = cos(w0);
	const float_t alpha = sinw0 * sinh(log(2.0f) * BW * w0 / sinw0);

	b0 = 1 - alpha;
	b1 = -2 * cosw0;
	b2 = 1 + alpha;
	a0 = 1 + alpha;
	a1 = -2 * cosw0;
	a2 = 1 - alpha;
}

void BiquadraticFilter::setPeakingParam(float_t sampleFreq,float_t centerFreq,float_t BW,float_t gain) 
{
	const float_t w0    = 2 * PI * centerFreq / sampleFreq;
	const float_t sinw0 = sin(w0);
	const float_t cosw0 = cos(w0);
	const float_t alpha = sinw0 * sinh(log(2.0f) * BW * w0 / sinw0);
	const float_t A     = (float_t)sqrt(pow(10, gain / 20));

	b0 = 1 + alpha * A;
	b1 = -2 * cosw0;
	b2 = 1 - alpha * A;
	a0 = 1 + alpha / A;
	a1 = -2 * cosw0;
	a2 = 1 - alpha / A;
}

void BiquadraticFilter::setLoshelfParam(float_t sampleFreq,float_t cutOffFreq,float_t S,float_t gain) 
{
	const float_t w0    = 2 * PI * cutOffFreq / sampleFreq;
	const float_t sinw0 = sin(w0);
	const float_t cosw0 = cos(w0);
	const float_t A     = (float_t)sqrt(pow(10, gain / 20));
	const float_t alpha = sinw0 / 2 * sqrt((A + 1 / A) * (1 / S - 1) + 2);
	const float_t sqrtA = sqrt(A);

	b0 = A * ((A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha);
	b1 = 2 * A * ((A - 1) - (A + 1) * cosw0);
	b2 = A * ((A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha);
	a0 = (A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha;
	a1 = -2 * ((A - 1) + (A + 1) * cosw0);
	a2 = (A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha;
}

void BiquadraticFilter::setHighshelfParam(float_t sampleFreq,float_t cutOffFreq,float_t S,float_t gain) 
{
	const float_t w0    = 2 * PI * cutOffFreq / sampleFreq;
	const float_t sinw0 = sin(w0);
	const float_t cosw0 = cos(w0);
	const float_t A     = (float_t)sqrt(pow(10, gain / 20));
	const float_t alpha = sinw0 / 2 * sqrt((A + 1 / A) * (1 / S - 1) + 2);
	const float_t sqrtA = sqrt(A);

	b0 = A * ((A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha);
	b1 = -2 * A * ((A - 1) + (A + 1) * cosw0);
	b2 = A * ((A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha);
	a0 = (A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha;
	a1 = 2 * ((A - 1) - (A + 1) * cosw0);
	a2 = (A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha;
}