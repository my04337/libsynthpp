#pragma once

#include <lsp/synth/instrument_table.hpp>
#include <filesystem>

namespace luath
{

// TOMLファイルからインストゥルメント情報を読み込みます
class InstrumentLoader final
{
public:
	// 指定ディレクトリ内の全 .toml ファイルを読み込み InstrumentTable を構築します
	// 各ファイルにはオプションで system_type, bank_msb, bank_lsb を記述可能
	static lsp::synth::InstrumentTable loadFromDirectory(const std::filesystem::path& dir);

private:
	static void loadFile(lsp::synth::InstrumentTable& table, const std::filesystem::path& path);
};

}
