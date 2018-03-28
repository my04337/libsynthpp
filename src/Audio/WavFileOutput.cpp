#include <LSP/Audio/WavFileOutput.hpp>
#include <LSP/Debugging/Logging.hpp>

using namespace LSP;

WavFileOutput::WavFileOutput()
{

}

WavFileOutput::~WavFileOutput()
{
	if (valid()) {
		finalize();
	}
}

bool WavFileOutput::valid() const noexcept
{
	return mFile.is_open() && !mFile.fail();
}

bool WavFileOutput::initialize(uint32_t sampleFreq, uint32_t bitsPerSample, uint32_t channels, const std::filesystem::path& filePath)
{
	if(valid()) {
		Log::e(LOGF("WavFileOutput : initialize - failed (already initialized)"));
		return false;
	}
	if(channels < 1 || channels > 2) {
		Log::e(LOGF("WavFileOutput : initialize - failed (invalid channel num)"));
		return false;
	}
	if(bitsPerSample != 16 && bitsPerSample != 32) {
		Log::e(LOGF("WavFileOutput : initialize - failed (invalid bits per sampe)"));
		return false;
	}

	std::ofstream fs;
	fs.open(filePath, std::ostream::out | std::ostream::trunc | std::ostream::binary);
	if (!fs) {
		Log::e(LOGF("WavFileOutput : initialize - failed (cannot open file)"));
		return false;
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
		Log::e(LOGF("WavFileOutput : initialize - failed (creating wav file header)"));
		return false;
	}

	// OK
	Log::i(LOGF("WavFileOutput : initialized"));
	mFile = std::move(fs);
	mSampleFreq = sampleFreq;
	mBitsPerSample = bitsPerSample;
	mChannels = channels;
	mFilePos_RiffSize = riff_size_pos;
	mFilePos_DataSize = data_size_pos;

	return true;
}

bool WavFileOutput::finalize()
{
	if(!mFile) {
		Log::e(LOGF("WavFileOutput : finalize - failed (not initialized)"));
		return false;
	}
	if(!valid()) {
		Log::e(LOGF("WavFileOutput : finalize - failed (not valid)"));
		return false;
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

	if(!valid()) {
		Log::e(LOGF("WavFileOutput : finalize - failed (file size)"));
		return false;
	}

	mFile.close();

	return true;
}

bool WavFileOutput::write(const std::vector<int32_t>& frame)
{
	const auto channels = mChannels; // for optimize
	const auto bitsPerSample = mBitsPerSample; 
	const auto bytesPerSample = bitsPerSample/8;

	lsp_assert(frame.size() == mChannels);

	if(!valid()) {
		Log::e(LOGF("WavFileOutput : write - failed (not valid)"));
		return false;
	}
	

	for (size_t ch = 0; ch < channels; ++ch) {
		int32_t s = frame[ch];
		s >>= 32 - bitsPerSample; // 32bitで記録しているので、必要サイズに併せて切り詰める
									// MEMO リトルエンディアン前提コード, 下位側から必要バイト分を転写
		mFile.write(reinterpret_cast<const char*>(&s), bytesPerSample);
	}

	return true;
}