#include "test_wasapi.hpp"
#include <LSP/Audio/WasapiOutput.hpp>
#include <LSP/Generator/NoiseGenerator.hpp>
#include <LSP/Filter/Requantizer.hpp>

#include <LSP/Base/Message.hpp>

using namespace LSP;


void Test::WasapiTest::exec()
{
	// Message
	Log::d("Testing : Wasapi");
	{
		Audio::WasapiOutput wo;
		if (!wo.valid()) {
			Log::d("Testing : Wasapi - failed (open)");
			return;
		}

		using sample_type = float;
		using requantize_from_float = Filter::Requantizer<float, sample_type>;
		const uint32_t sampleFreq = wo.getDeviceSampleFreq();
		const size_t bufferFrameCount = wo.getDeviceBufferFrameCount();
		const int64_t maxFrameCount = sampleFreq * 3; // 3秒

		Generator::NoiseGenerator<sample_type, Generator::NoiseColor::White> noisegen1(sampleFreq);
		Generator::NoiseGenerator<sample_type, Generator::NoiseColor::Brown> noisegen2(sampleFreq);
		std::pmr::unsynchronized_pool_resource mem;
		int64_t time = 0;

		if(!wo.start()) {
			Log::w("Testing : Wasapi - failed (start)");
			return;
		}
		while(true) {
			if (wo.getBufferedFrameCount() < bufferFrameCount) {
				auto sig = Signal<sample_type>::allocate(&mem, 2, bufferFrameCount);
				for (uint32_t i = 0; i < bufferFrameCount; ++i) {
					float p = (time % sampleFreq) * 2.0f * PI<float> / sampleFreq; // 位相
					auto frame = sig.frame(i);
					frame[0] = requantize_from_float()( noisegen1.generate() );
					frame[1] = requantize_from_float()( noisegen2.generate() );
					++time;
					if(time >= maxFrameCount) break;
				}
				wo.write(sig);
				continue;
			}
			if(time >= maxFrameCount) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		if(!wo.stop()) {
			Log::w("Testing : Wasapi - failed (stop)");
			return;
		}
	}

	Log::d("Testing : Wasapi - End");
}