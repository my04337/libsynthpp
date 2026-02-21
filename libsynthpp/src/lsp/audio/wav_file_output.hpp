#pragma once

#include <lsp/core/core.hpp>

#include <fstream>
#include <filesystem>

namespace lsp::audio {

// WavFileOutput
class WavFileOutput final
	: non_copy_move
{
public:
	WavFileOutput(uint32_t sampleFreq, uint32_t bitsPerSample, uint32_t channels, const std::filesystem::path& filePath);
	~WavFileOutput();

	// ファイルへの出力を終了します
	void close()noexcept;

	// 何らかの異常が発生しているか否かを取得します
	bool fail() const noexcept;
	// 読み込み/書き込みエラーが発生しているか否かを取得します
	bool bad() const noexcept;

	// 信号を書き込みます
	template<typename sample_type>
	void write(const Signal<sample_type>& sig);
	

private:
	// --- valid時のみ有効 ---
	uint32_t mSampleFreq;
	uint32_t mBitsPerSample;
	uint32_t mChannels;
	
	std::ofstream mFile;
	std::ofstream::pos_type mFilePos_RiffSize;
	std::ofstream::pos_type mFilePos_DataSize;

};

// ---

template<typename sample_type>
void WavFileOutput::write(const Signal<sample_type>& sig)
{
	const auto signal_channels = sig.channels();
	const auto signal_frames = sig.frames();

	if(signal_channels == 0) return;
	if(signal_frames == 0) return;

	if (fail()) {
		Log::e("WavFileOutput : write - failed (invalid)");
		return;
	}
	lsp_require(signal_channels == mChannels, "WavFileOutput : write - failed (channel count is mismatch)");

	const auto bitsPerSample = mBitsPerSample; 
	const auto bytesPerSample = bitsPerSample/8;
	
	for(size_t i=0; i<signal_frames; ++i) {
		auto in_frame = sig.frame(i);
		for (size_t ch=0; ch< signal_channels; ++ch) {
			// 32bit整数型に変換
			auto s = requantize<int32_t>(in_frame[ch]);

			// 32bitで記録しているので、必要サイズに併せて切り詰める
			// MEMO リトルエンディアン前提コード, 下位側から必要バイト分を転写
			s >>= 32 - bitsPerSample; 

			// ファイルへ書き込み
			mFile.write(reinterpret_cast<const char*>(&s), bytesPerSample);
		}
	}
}

}