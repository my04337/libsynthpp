#pragma once

#include <LSP/minimal.hpp>

#include <fstream>
#include <filesystem>

namespace LSP::Audio {

// WASAPI 出力
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
	template<typename sample_type, typename Tintermediate = double>
	void write(const Signal<sample_type>& sig);

protected:
	void write(const std::vector<int32_t>& frame);


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

template<typename sample_type, typename Tintermediate>
void WavFileOutput::write(const Signal<sample_type>& sig)
{
	const auto signal_channels = sig.channels();
	const auto signal_length = sig.length();

	if(signal_channels == 0) return;
	if(signal_length == 0) return;

	if (fail()) {
		Log::e(LOGF("WasapiOutput : write - failed (invalid)"));
		return;
	}
	lsp_assert_desc(signal_channels == mChannels, "WasapiOutput : write - failed (channel count is mismatch)");
	
	std::vector<int32_t> frame;
	frame.resize(signal_channels);

	for(size_t i=0; i<signal_length; ++i) {
		for (size_t ch=0; ch< signal_channels; ++ch) {
			auto s = SampleFormatConverter<sample_type, int32_t, Tintermediate>::convert(sig.data(ch)[i]);
			frame[ch] = s;
		}
		write(frame);
	}
}

}