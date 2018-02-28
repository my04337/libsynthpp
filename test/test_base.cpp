#include <LSP/minimal.hpp>

// テンプレート実体化チェック

namespace {

void uncalled_func()
{
	constexpr auto pi1 = LSP::PI<float>;
	constexpr auto pi2 = LSP::PI<double>;
	constexpr auto pi3 = LSP::PI<long double>;

	LSP::Signal<float> signal1(0);
	LSP::Signal<double> signal2(0);
	LSP::Signal<long double> signal3(0);
	LSP::Signal<int32_t> signal4(0);
	LSP::Signal<uint16_t> signal5(0);
	
	static_assert(LSP::is_signal_type_v<LSP::Signal<float>> == true, "");
	static_assert(LSP::is_signal_type_v<void> == false, "");
}

}