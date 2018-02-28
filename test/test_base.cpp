#include <LSP/minimal.hpp>
#include <LSP/Util/BiquadraticFilter.hpp>

// テンプレート実体化チェック

namespace {

void uncalled_func()
{
	constexpr auto pi1 = LSP::PI<float>;
	constexpr auto pi2 = LSP::PI<double>;
	constexpr auto pi3 = LSP::PI<long double>;

	LSP::Signal<float> signal_float(0);
	LSP::Signal<double> signal_double(0);
	LSP::Signal<long double> signal_long_double(0);
	LSP::Signal<int32_t> signal_int32_t(0);
	LSP::Signal<uint16_t> signal_uint16_t(0);
	
	static_assert(LSP::is_signal_type_v<LSP::Signal<float>> == true, "");
	static_assert(LSP::is_signal_type_v<void> == false, "");

	LSP::BiquadraticFilter<float> bqf_float;
	LSP::BiquadraticFilter<double> bqf_double;
	LSP::BiquadraticFilter<long double> bqf_long_double;

	LSP::BiquadraticFilter<float, double> bqf_float_and_double;
}

}