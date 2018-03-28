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
	template<typename signal_type, typename Tintermediate = double>
	void write(const Signal<signal_type>& ch1);
	template<typename signal_type, typename Tintermediate = double>
	void write(const Signal<signal_type>& ch1, const Signal<signal_type>& ch2);
	template<typename signal_type, typename Tintermediate = double>
	void write(const Signal<signal_type>* sigs[], size_t num);

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

template<typename signal_type, typename Tintermediate>
void WavFileOutput::write(const Signal<signal_type>& ch1)
{
	const Signal<signal_type>* sigs[] = {&ch1};
	write<signal_type, Tintermediate>(sigs, 1);
}
template<typename signal_type, typename Tintermediate>
void WavFileOutput::write(const Signal<signal_type>& ch1, const Signal<signal_type>& ch2)
{
	const Signal<signal_type>* sigs[] = {&ch1, &ch2};
	write<signal_type, Tintermediate>(sigs, 2);
}

template<typename signal_type, typename Tintermediate>
void WavFileOutput::write(const Signal<signal_type>* sigs[], size_t num)
{
	if(num == 0) return;

	if (fail()) {
		Log::e(LOGF("WasapiOutput : write - failed (invalid)"));
		return;
	}
	lsp_assert_desc(num == mChannels, "WasapiOutput : write - failed (channel count is mismatch)");

	const auto sz = sigs[0]->size();
	for (size_t ch=1; ch< num; ++ch) {
		lsp_assert_desc(sigs[ch]->size() == sz, "WasapiOutput : write - failed (signal length is mismatch)");
	}
	if(sz == 0) return;

	std::vector<int32_t> frame;
	frame.resize(num);

	for(size_t i=0; i<sz; ++i) {
		for (size_t ch=0; ch< num; ++ch) {
			auto s = SampleFormatConverter<signal_type, int32_t, Tintermediate>::convert(sigs[ch]->data()[i]);
			frame[ch] = s;
		}
		write(frame);
	}
}

}