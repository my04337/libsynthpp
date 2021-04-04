#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>

namespace LSP::Util::FFT
{
// 各種窓関数
float BlackmanWf(float pos) 
{
	if (pos < 0 || pos > 1) return 0;
	return 0.42 - 0.46 * std::cos(2 * PI<float> * pos) + 0.08 * std::cos(4 * PI<float> * pos);
}
float BlackmanHarrisWf(float pos)
{
	if (pos < 0 || pos > 1) return 0;
	return 0.35875 - 0.48829 * std::cos(pos) + 0.14128 * std::cos(2 * pos) - 0.01168 * std::cos(3 * pos);
}
float RectangularWf(float pos)
{
	if (pos < 0 || pos > 1) return 0;
	return 1;
}
float GaussW(float pos)
{
	if (pos < 0 || pos > 1) return 0;
	return std::exp(-(pos * pos) / (0.4 * 0.4));
}
float HammingWf(float pos)
{
	if (pos < 0 || pos > 1) return 0;
	return 0.5 - 0.5 * (std::cos(2 * PI<float> * pos));
}
float HanningWf(float pos)
{
	if (pos < 0 || pos > 1) return 0;
	return 0.54 - 0.46 * (std::cos(2 * PI<float> * pos));
}

template<floating_sample_typeable sample_type, subscript_operator_available<sample_type> container>
bool fft1d(container& ar, container& ai, int n, int iter, bool isIFFT)
{
	int i, it, j, j1, j2, k, xp, xp2;
	sample_type arg, dr1, dr2, di1, di2, tr, ti, w, wr, wi;

	if (n < 2)
	{
		return false;
	}
	if (iter <= 0)
	{
		iter = 0;
		i = n;
		while ((i /= 2) != 0)	iter++;
	}
	j = 1;
	for (i = 0; i < iter; i++)	j *= 2;
	if (n != j)
	{
		return false;
	}
	w = (isIFFT ? PI<sample_type> : -PI<sample_type>) / (sample_type)n;
	xp2 = n;
	for (it = 0; it < iter; it++)
	{
		xp = xp2;
		xp2 /= 2;
		w *= 2;
		for (k = 0, i = -xp; k < xp2; i++)
		{
			wr = std::cos(arg = w * k++);
			wi = std::sin(arg);
			for (j = xp; j <= n; j += xp)
			{
				j2 = (j1 = j + i) + xp2;
				tr = (dr1 = ar[j1]) - (dr2 = ar[j2]);
				ti = (di1 = ai[j1]) - (di2 = ai[j2]);
				ar[j1] = dr1 + dr2;
				ai[j1] = di1 + di2;
				ar[j2] = tr * wr - ti * wi;
				ai[j2] = ti * wr + tr * wi;
			}
		}
	}
	j = j1 = n / 2;
	j2 = n - 1;
	for (i = 1; i < j2; i++)
	{
		if (i < j)
		{
			w = ar[i];
			ar[i] = ar[j];
			ar[j] = w;
			w = ai[i];
			ai[i] = ai[j];
			ai[j] = w;
		}
		k = j1;
		while (k <= j)
		{
			j -= k;
			k /= 2;
		}
		j += k;
	}
	if (!isIFFT)	return true;
	w = 1 / (sample_type)n;
	for (i = 0; i < n; i++)
	{
		ar[i] *= w;
		ai[i] *= w;
	}
	return true;
}

}