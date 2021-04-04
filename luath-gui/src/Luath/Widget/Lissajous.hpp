#pragma once

#include <Luath/Base/Base.hpp>

namespace Luath::Widget
{

class Lissajous
{
public:
	Lissajous();
	~Lissajous();

	// �\���p�����[�^��ݒ肵�܂�
	void setParam(uint32_t sampleFreq, uint32_t channels, uint32_t bufferLength);

	// �\���g�`���������݂܂�
	template<typename sample_type>
	void write(const LSP::Signal<sample_type>& sig);


	// ���T�[�W���Ȑ���`���`�悵�܂�
	void draw(SDL_Renderer* renderer, int x, int y, int width, int height);
	
private:
	void _reset();

	mutable std::mutex mMutex;
	uint32_t mSampleFreq = 1;
	uint32_t mChannels = 1; // ��M�`���l��
	uint32_t mBufferLength = 1;
	std::vector<std::deque<float>> mBuffers; // �����O�o�b�t�@
};

template<typename sample_type>
void Lissajous::write(const LSP::Signal<sample_type>& sig)
{
	std::lock_guard lock(mMutex);

	const auto signal_channels = sig.channels();
	const auto signal_frames = sig.frames();
	const auto buffer_length = mBufferLength;

	lsp_assert_desc(signal_channels == mChannels, "WasapiOutput : write - failed (channel count is mismatch)");


	for (size_t ch=0; ch<signal_channels; ++ch) {
		auto& buff = mBuffers[ch];
		for(size_t i=0; i<signal_frames; ++i) {
			buff.push_back(requantize<float>(sig.frame(i)[ch]));
		}
		while(buff.size() > buffer_length) buff.pop_front();
	}
}

//
}