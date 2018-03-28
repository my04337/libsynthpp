#include <LSP/minimal.hpp>
#include <LSP/Audio/WasapiOutput.hpp>
#include <LSP/Audio/WavFileOutput.hpp>
#include <LSP/Generator/SinOscillator.hpp>
#include <LSP/Generator/NoiseGenerator.hpp>
#include <LSP/Filter/BiquadraticFilter.hpp>

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

	using sample_type = float;
	using conv_from_float = SampleFormatConverter<float, sample_type>;
	const uint32_t sampleFreq = wo.getDeviceSampleFreq();
	const size_t bufferFrameCount = wo.getDeviceBufferFrameCount();

	Generator::NoiseGenerator<sample_type, Generator::NoiseColor::White> noisegen1(sampleFreq);
	Generator::NoiseGenerator<sample_type, Generator::NoiseColor::White> noisegen2(sampleFreq);
	Filter::BiquadraticFilter<double> bqf2;
	bqf2.setLopassParam(sampleFreq, 1000, 1);
	Signal<sample_type> sig_left(bufferFrameCount);
	Signal<sample_type> sig_right(bufferFrameCount);
	int64_t time = 0;
	
	if(!wo.start()) {
		return -1;
	}
	while(true) {
		if (wo.getBufferedFrameCount() < bufferFrameCount) {
			for (uint32_t i = 0; i < bufferFrameCount; ++i) {
				float p = (time % sampleFreq) * 2.0f * PI<float> / sampleFreq; // 位相
				sig_left.data()[i]  = conv_from_float()( noisegen1.generate() );
				sig_right.data()[i] = conv_from_float()( bqf2.update(noisegen2.generate()) );
				++time;
			}
			wo.write(sig_left, sig_right);
			continue;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}


	return 0;
}