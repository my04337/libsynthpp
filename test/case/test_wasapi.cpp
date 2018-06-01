#include "test_wasapi.hpp"
#include <LSP/Audio/WasapiOutput.hpp>
#include <LSP/Generator/FunctionGenerator.hpp>
#include <LSP/Filter/Requantizer.hpp>
#include <LSP/Filter/EnvelopeGenerator.hpp>

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
		constexpr auto to_sample_type = Filter::Requantizer<sample_type>();
		const uint32_t sampleFreq = wo.getDeviceSampleFreq();
		const size_t bufferFrameCount = wo.getDeviceBufferFrameCount();
		const int64_t maxFrameCount = static_cast<int64_t>(sampleFreq * 1.5); 

		Generator::FunctionGenerator<sample_type> osc;
		Filter::EnvelopeGenerator<sample_type> eg;
		std::pmr::unsynchronized_pool_resource mem;
		int64_t time = 0;

		osc.setBrownNoise();
		eg.setParam((sample_type)sampleFreq, {}, 0.2f, 0.1f, 0.05f, 0.6f, -0.5f, 0.3f);

		if(!wo.start()) {
			Log::w("Testing : Wasapi - failed (start)");
			return;
		}
		while(true) {
			if (wo.getBufferedFrameCount() < bufferFrameCount) {
				auto sig = Signal<sample_type>::allocate(&mem, 2, bufferFrameCount);
				for (uint32_t i = 0; i < bufferFrameCount; ++i) {
					if (time == sampleFreq*0.2) {
						eg.noteOn();
					}
					else if (time == sampleFreq * 0.8) {
						eg.noteOff();
					}
					float p = (time % sampleFreq) * 2.0f * PI<float> / sampleFreq; // 位相
					auto frame = sig.frame(i);
					frame[0] = to_sample_type( osc.update() );
					frame[1] = to_sample_type( eg.update() );
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