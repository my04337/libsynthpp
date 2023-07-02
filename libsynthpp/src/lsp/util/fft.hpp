/**
	libsynth++

	Copyright(c) 2018 my04337

	This software is released under the MIT License.
	https://opensource.org/license/mit/
*/

#pragma once

#include <lsp/core/core.hpp>

namespace lsp::fft
{
// 各種窓関数
template<
	std::floating_point parameter_type
>
parameter_type BlackmanWf(parameter_type pos)
{
	// https://en.wikipedia.org/wiki/Window_function#Blackman_window
	constexpr parameter_type alpha = 0.16f;
	constexpr parameter_type a0 = (1.0f - alpha) / 2.0f;
	constexpr parameter_type a1 = 0.5f;
	constexpr parameter_type a2 = alpha / 2.0f;

	if (pos < 0 || pos > 1) return 0;
	return a0 - a1 * std::cos(2 * math::PI<parameter_type> * pos) + a2 * std::cos(4 * math::PI<parameter_type> * pos);
}
template<
	std::floating_point parameter_type
>
parameter_type BlackmanHarrisWf(parameter_type pos)
{
	constexpr parameter_type a0 = 0.35875f;
	constexpr parameter_type a1 = 0.48829f;
	constexpr parameter_type a2 = 0.14128f;
	constexpr parameter_type a3 = 0.01168f;

	if (pos < 0 || pos > 1) return 0;
	return a0 - a1 * std::cos(2 * math::PI<parameter_type> * pos) + a2 * std::cos(4 * math::PI<parameter_type> * pos) - a3 * std::cos(4 * math::PI<parameter_type> *pos);
}
template<
	std::floating_point parameter_type
>
parameter_type RectangularWf(parameter_type pos)
{
	if (pos < 0 || pos > 1) return 0;
	return 1;
}
template<
	std::floating_point parameter_type
>
parameter_type HannWf(parameter_type pos)
{
	constexpr parameter_type a0 = 0.5f;
	constexpr parameter_type a1 = 1.0f - a0;

	if (pos < 0 || pos > 1) return 0;
	return a0 - a1 * (std::cos(2 * math::PI<parameter_type> *pos));
}
template<
	std::floating_point parameter_type
>
parameter_type HammingWf(parameter_type pos)
{
	constexpr parameter_type a0 = 25.0f / 46.0f;
	constexpr parameter_type a1 = 1.0f - a0;

	if (pos < 0 || pos > 1) return 0;
	return a0 - a1 * (std::cos(2 * math::PI<parameter_type> * pos));
}

template<std::floating_point sample_type>
bool fft1d(sample_type* ar, sample_type* ai, int n, int iter, bool isIFFT)
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
	w = (isIFFT ? math::PI<sample_type> : -math::PI<sample_type>) / (sample_type)n;
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