#include <LSP/Audio/SDLOutput.hpp>
#include <SDL_audio.h>


using namespace LSP;
using namespace LSP::Audio;

SDLOutput::SDLOutput(uint32_t sampleFreq, uint32_t channels, SampleFormat format)
	: mAudioDeviceID(0)
	, mAudioBufferFrameCount(0)
	, mSampleFormat(SampleFormat::Unknown)
{
	initialize(sampleFreq, channels, format);
}
SDLOutput::~SDLOutput()
{	// オーディオデバイス 破棄
	if (mAudioDeviceID) {
		SDL_CloseAudioDevice(mAudioDeviceID);
		mAudioDeviceID = 0;
	}
}
bool SDLOutput::valid() const noexcept
{
	return mAudioDeviceID > 0;
}
void SDLOutput::initialize(uint32_t sampleFreq, uint32_t channels, SampleFormat format)
{
	if(valid()) {
		Log::e(LOGF("SDLOutput : initialize - failed (already initialized)"));
		return;
	}

	// 再生デバイス情報組み立て
	SDL_AudioSpec want, have;
	SDL_memset(&want, 0, sizeof(want));

	want.freq = static_cast<int>(sampleFreq);
	want.channels = static_cast<Uint8>(channels);
	uint32_t bytePerSample;
	switch (format) {
	case SampleFormat::Int8:	want.format = AUDIO_S8;	 bytePerSample = 1; break;
	case SampleFormat::Int16:	want.format = AUDIO_S16; bytePerSample = 2; break;
	case SampleFormat::Int32:	want.format = AUDIO_S32; bytePerSample = 4; break;
	case SampleFormat::Float32:	want.format = AUDIO_F32; bytePerSample = 4; break;
	case SampleFormat::Unknown:	
	default:					want.format = AUDIO_S16; bytePerSample = 2; break;
	}
	
	want.samples = 4096;
	
	auto deviceId = SDL_OpenAudioDevice(
		nullptr, 0,
		&want, &have, 
		0
	);
	if (deviceId == 0) {
		Log::e(LOGF("SDLOutput : initialize - failed (openDevice)[" << SDL_GetError() << "]"));
		return;
	}

	// OK
	mAudioDeviceID = deviceId;
	mAudioSpec = std::make_unique<SDL_AudioSpec>(have);
	mSampleFormat = format;
	mBytesPerSample = bytePerSample;
	Log::i(LOGF("SDLOutput : initialized"));
	Log::i(LOGF("  sample freq => " << mAudioSpec->freq << "[Hz]"));
	Log::i(LOGF("  channels    => " << mAudioSpec->channels));
	Log::i(LOGF("  format      => " << getDeviceFormatString(mSampleFormat)));
	SDL_PauseAudioDevice(mAudioDeviceID, 0);
}
void SDLOutput::_write(void* data, size_t len)
{
	lsp_assert(valid());
	SDL_QueueAudio(mAudioDeviceID, data, static_cast<Uint32>(len));
}
bool SDLOutput::start()
{
	if (!valid()) {
		Log::e(LOGF("SDLOutput : start - failed (invalid)"));
		return false;
	}
	SDL_PauseAudioDevice(mAudioDeviceID, 0);
	return true;
}

bool SDLOutput::stop()
{
	if (!valid()) {
		Log::e(LOGF("SDLOutput : stop - failed (invalid)"));
		return false;
	}
	SDL_PauseAudioDevice(mAudioDeviceID, 1);
	return true;
}
uint32_t SDLOutput::getDeviceSampleFreq()const noexcept
{
	lsp_assert(valid());
	return static_cast<uint32_t>(mAudioSpec->freq);
}
uint32_t SDLOutput::getDeviceChannels()const noexcept
{
	lsp_assert(valid());
	return static_cast<uint32_t>(mAudioSpec->channels);
}

SDLOutput::SampleFormat SDLOutput::getDeviceFormat()const noexcept
{
	lsp_assert(valid());
	return mSampleFormat;
}
std::string_view SDLOutput::getDeviceFormatString()const noexcept
{
	lsp_assert(valid());
	return getDeviceFormatString(mSampleFormat);
}
std::string_view SDLOutput::getDeviceFormatString(SampleFormat format)noexcept
{
	switch (format) {
	case SampleFormat::Unknown:	return "Unknown";
	case SampleFormat::Int8:	return "Int8";
	case SampleFormat::Int16:	return "Int16";
	case SampleFormat::Int32:	return "Int32";
	case SampleFormat::Float32:	return "Float32";
	}
	lsp_assert(false);
}

size_t SDLOutput::getDeviceBufferFrameCount()const noexcept
{
	lsp_assert(valid());
	return static_cast<size_t>(mAudioSpec->samples);
}

size_t SDLOutput::getBufferedFrameCount()const noexcept
{
	lsp_assert(valid());
	auto buffered_bytes = SDL_GetQueuedAudioSize(mAudioDeviceID);
	return buffered_bytes / mBytesPerSample / mAudioSpec->channels;
}