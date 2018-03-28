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
	Audio::WasapiOutput wo;
	if (!wo.valid()) {
		return -1;
	}

	const uint32_t sampleFreq = wo.getDeviceSampleFreq();
	const uint32_t bufferFrameCount = wo.getDeviceBufferFrameCount();
		
	Signal<float> sig_left(bufferFrameCount);
	Signal<float> sig_right(bufferFrameCount);
	int64_t time = 0;
	
	if(!wo.start()) {
		return -1;
	}
	while(true) {
		if (wo.getBufferedFrameCount() < bufferFrameCount) {
			for (uint32_t i = 0; i < bufferFrameCount; ++i) {
				float p = (time % sampleFreq) * 2.0f * PI<float> / sampleFreq; // 位相
				sig_left.data()[i]  = sin(440.0f * p)/3 + sin(660.0f * p)/4;
				sig_right.data()[i] = sin(880.0f * p)/2;
				++time;
			}
			wo.write(sig_left, sig_right);
			continue;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}


	return 0;
}