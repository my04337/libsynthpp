#include <lsp/audio/wav_file_output.hpp>

using namespace lsp;
using namespace lsp::audio;

WavFileOutput::WavFileOutput(uint32_t sampleFreq, uint32_t bitsPerSample, uint32_t channels, const std::filesystem::path& filePath)
{
	if(channels < 1 || channels > 2) {
		Log::e("WavFileOutput : initialize - failed (invalid channel num)");
		return;
	}
	if(bitsPerSample != 16 && bitsPerSample != 32) {
		Log::e("WavFileOutput : initialize - failed (invalid bits per sampe)");
		return;
	}

	std::ofstream fs;
	fs.open(filePath, std::ostream::out | std::ostream::trunc | std::ostream::binary);
	if (!fs) {
		Log::e("WavFileOutput : initialize - failed (cannot open file)");
		return;
	}

	auto write_binary = [&fs](const auto v) {
		fs.write(reinterpret_cast<const char*>(&v), sizeof(v));
	};


	// TODO リトルエンディアンのPC専用
	const uint32_t bytesPerSample = bitsPerSample / 8;
	const uint32_t blockSize = bytesPerSample * channels;
	const uint32_t bytesPerSec = sampleFreq * blockSize;

	// --- RIFF ファイルヘッダ ---
	fs << "RIFF";
	auto riff_size_pos = fs.tellp();
	write_binary(uint32_t(0)); // [プレースホルダ] これ以降のファイルサイズ
	fs << "WAVE";

	// --- fmtチャンク ---
	fs << "fmt ";
	write_binary(uint32_t(16)); // fmtチャンク バイト数
	write_binary(uint16_t(0x01)); // PCMフォーマット
	write_binary(uint16_t(channels)); // チャネル数
	write_binary(uint32_t(sampleFreq)); // サンプリングレート
	write_binary(uint32_t(bytesPerSec)); // データ速度
	write_binary(uint16_t(blockSize)); // ブロックサイズ
	write_binary(uint16_t(bitsPerSample)); // サンプル当たりのビット数

	// --- dataチャンク ---
	fs << "data";
	auto data_size_pos = fs.tellp();
	write_binary(uint32_t(0)); // [プレースホルダ] 波形データのバイト数

	// ---

	if (!fs) {
		Log::e("WavFileOutput : initialize - failed (creating wav file header)");
		return;
	}

	// OK
	Log::i("WavFileOutput : initialized");
	mFile = std::move(fs);
	mSampleFreq = sampleFreq;
	mBitsPerSample = bitsPerSample;
	mChannels = channels;
	mFilePos_RiffSize = riff_size_pos;
	mFilePos_DataSize = data_size_pos;

}

WavFileOutput::~WavFileOutput()
{
	close();
}

bool WavFileOutput::fail() const noexcept
{
	return mFile.fail();
}

bool WavFileOutput::bad() const noexcept
{
	return mFile.bad();
}


void WavFileOutput::close()noexcept
{
	// すでにファイルが閉じられている場合、何もする必要はない
	if(!mFile.is_open()) return ;

	// このメソッドから帰る場合、どのような場合でもファイルを閉じる
	auto fin_act_close_file = finally([this]{ mFile.close(); });

	// ---
	if(!mFile) {
		Log::e("WavFileOutput : finalize - failed (not valid)");
		return;
	}

	auto write_binary = [this](const auto v) {
		mFile.write(reinterpret_cast<const char*>(&v), sizeof(v));
	};

	auto current_pos = mFile.tellp();
	auto data_size = current_pos - mFilePos_DataSize - 4;
	mFile.seekp(mFilePos_DataSize);
	write_binary(uint32_t(data_size));

	auto riff_size = current_pos - mFilePos_RiffSize - 4;
	mFile.seekp(mFilePos_RiffSize);
	write_binary(uint32_t(riff_size));

	if(!mFile) {
		Log::e("WavFileOutput : finalize - failed (file size)");
		return;
	}

	mFile.close();
}
