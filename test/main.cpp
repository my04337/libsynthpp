#include <LSP/minimal.hpp>
#include <LSP/Audio/WasapiOutput.hpp>
#include <LSP/Audio/WavFileOutput.hpp>

#ifdef WIN32
#include <objbase.h>
#endif

using namespace LSP;

void func() {
	lsp_debug_log("");
}

int main(int argc, char** argv)
{
#ifdef WIN32
	// COM初期化
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	auto fin_act_com = finally([]{CoUninitialize();});
#endif

	// ログ出力機構 セットアップ
	StdOutLogger logger;
	Log::addLogger(&logger);
	auto fin_act_logger = finally([&]{ Log::removeLogger(&logger); });
	Log::setLogLevel(LogLevel::Debug);

	// ---

	constexpr uint32_t sampleFreq = 44100;
	constexpr uint32_t unitSampleCount = 4410;
	Signal<float> sig_left(unitSampleCount);
	Signal<float> sig_right(unitSampleCount);
	int64_t time = 0;
	
	Audio::WasapiOutput wo;
	if (!wo.initialize(sampleFreq, 16, 2)) {
		return -1;
	}
	if(!wo.start()) {
		return -1;
	}
	while(true) {
		if (wo.buffered_count() < unitSampleCount) {
			for (uint32_t i = 0; i < unitSampleCount; ++i) {
				float p = (time % unitSampleCount) * 2.0f * PI<float> / sampleFreq; // 位相
				sig_left.data()[i]  = sin(440.0f * p);
				sig_right.data()[i] = sin(880.0f * p);
				++time;
			}
			wo.write(sig_left, sig_right);
			continue;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}


	return 0;
}