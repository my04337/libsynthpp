#include <luath/io/instrument_loader.hpp>
#include <toml++/toml.hpp>

using namespace luath;
using namespace lsp::synth;

// 文字列からMelodyWaveFormへ変換
static MelodyWaveForm parseWaveForm(std::string_view str)
{
	if(str == "sine")     return MelodyWaveForm::Sine;
	if(str == "triangle") return MelodyWaveForm::Triangle;
	if(str == "sawtooth") return MelodyWaveForm::Sawtooth;
	if(str == "noise")    return MelodyWaveForm::Noise;
	return MelodyWaveForm::Square; // デフォルト
}

// 文字列からInstrumentSystemTypeへ変換
static InstrumentSystemType parseSystemType(std::string_view str)
{
	if(str == "GM1") return InstrumentSystemType::GM1;
	if(str == "GM2") return InstrumentSystemType::GM2;
	if(str == "GS")  return InstrumentSystemType::GS;
	if(str == "XG")  return InstrumentSystemType::XG;
	return InstrumentSystemType::None;
}

// ファイルレベルのデフォルト variant key を読み取る
static InstrumentVariantKey readFileDefaults(const toml::table& tbl)
{
	InstrumentVariantKey key;
	key.systemType = parseSystemType(tbl["system_type"].value_or(std::string{}));
	key.bankMSB    = static_cast<uint8_t>(tbl["bank_msb"].value_or(0));
	key.bankLSB    = static_cast<uint8_t>(tbl["bank_lsb"].value_or(0));
	return key;
}

// 個別エントリの variant key を読み取る (ファイルデフォルトをベースにオーバーライド)
static InstrumentVariantKey readEntryVariant(const toml::table& entry, const InstrumentVariantKey& fileDefault)
{
	InstrumentVariantKey key = fileDefault;
	if(auto v = entry["system_type"].value<std::string>()) key.systemType = parseSystemType(*v);
	return key;
}

// 整数キーとして解釈可能か判定し、値を返す
static std::optional<int32_t> tryParseInt(std::string_view str)
{
	try {
		return static_cast<int32_t>(std::stoi(std::string(str)));
	} catch(...) {
		return std::nullopt;
	}
}

InstrumentTable InstrumentLoader::loadFromDirectory(const std::filesystem::path& dir)
{
	InstrumentTable table;

	if(!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
		return table;
	}

	// ディレクトリ内の全 .toml ファイルを読み込む
	for(auto& entry : std::filesystem::directory_iterator(dir)) {
		if(!entry.is_regular_file()) continue;
		if(entry.path().extension() != ".toml") continue;
		loadFile(table, entry.path());
	}

	return table;
}

void InstrumentLoader::loadFile(InstrumentTable& table, const std::filesystem::path& path)
{
	auto tbl = toml::parse_file(path.string());

	// ファイルレベルのデフォルト variant key
	auto fileDefault = readFileDefaults(tbl);

	// --- [melody.*] セクションを処理 ---
	// キー形式:
	//   [melody.{progId}]                  → bank 0/0 (ファイルデフォルト)
	//   [melody.{progId}.{bankMSB}.{bankLSB}] → 指定バンク
	if(auto melody = tbl["melody"].as_table()) {
		for(auto& [progKey, progValue] : *melody) {
			auto progId = tryParseInt(progKey.str());
			if(!progId) continue;

			auto progTable = progValue.as_table();
			if(!progTable) continue;

			// パラメータフィールド(volume等)が直接あればbank 0/0のエントリ
			if(progTable->contains("volume") || progTable->contains("attack") || progTable->contains("decay")) {
				auto variant = readEntryVariant(*progTable, fileDefault);

				MelodyParam param;
				param.caption    = (*progTable)["caption"].value_or(std::string{});
				param.volume     = (*progTable)["volume"].value_or(1.00f);
				param.attack     = (*progTable)["attack"].value_or(0.02f);
				param.hold       = (*progTable)["hold"].value_or(0.00f);
				param.decay      = (*progTable)["decay"].value_or(3.00f);
				param.sustain    = (*progTable)["sustain"].value_or(0.00f);
				param.fade       = (*progTable)["fade"].value_or(0.00f);
				param.release    = (*progTable)["release"].value_or(1.00f);
				param.waveForm   = parseWaveForm((*progTable)["wave_form"].value_or(std::string{"square"}));
				param.isDrumLike = (*progTable)["drum_like"].value_or(false);
				param.noteOffset = (*progTable)["note_offset"].value_or(0.0f);

				table.setMelodyParam(variant, *progId, param);
			}

			// サブテーブル [melody.{progId}.{bankMSB}.*] を走査
			for(auto& [msbKey, msbValue] : *progTable) {
				auto bankMSB = tryParseInt(msbKey.str());
				if(!bankMSB) continue;

				auto msbTable = msbValue.as_table();
				if(!msbTable) continue;

				for(auto& [lsbKey, lsbValue] : *msbTable) {
					auto bankLSB = tryParseInt(lsbKey.str());
					if(!bankLSB) continue;

					auto lsbTable = lsbValue.as_table();
					if(!lsbTable) continue;

					auto variant = readEntryVariant(*lsbTable, fileDefault);
					variant.bankMSB = static_cast<uint8_t>(*bankMSB);
					variant.bankLSB = static_cast<uint8_t>(*bankLSB);

					MelodyParam param;
					param.caption    = (*lsbTable)["caption"].value_or(std::string{});
					param.volume     = (*lsbTable)["volume"].value_or(1.00f);
					param.attack     = (*lsbTable)["attack"].value_or(0.02f);
					param.hold       = (*lsbTable)["hold"].value_or(0.00f);
					param.decay      = (*lsbTable)["decay"].value_or(3.00f);
					param.sustain    = (*lsbTable)["sustain"].value_or(0.00f);
					param.fade       = (*lsbTable)["fade"].value_or(0.00f);
					param.release    = (*lsbTable)["release"].value_or(1.00f);
					param.waveForm   = parseWaveForm((*lsbTable)["wave_form"].value_or(std::string{"square"}));
					param.isDrumLike = (*lsbTable)["drum_like"].value_or(false);
					param.noteOffset = (*lsbTable)["note_offset"].value_or(0.0f);

					table.setMelodyParam(variant, *progId, param);
				}
			}
		}
	}

	// --- [drum.*] セクションを処理 ---
	// キー形式:
	//   [drum.{noteNo}]                      → bank 0/0 (ファイルデフォルト)
	//   [drum.{noteNo}.{bankMSB}.{bankLSB}]  → 指定バンク
	if(auto drum = tbl["drum"].as_table()) {
		for(auto& [noteKey, noteValue] : *drum) {
			auto noteNo = tryParseInt(noteKey.str());
			if(!noteNo) continue;

			auto noteTable = noteValue.as_table();
			if(!noteTable) continue;

			// パラメータフィールドが直接あればbank 0/0のエントリ
			if(noteTable->contains("pitch") || noteTable->contains("volume") || noteTable->contains("decay")) {
				auto variant = readEntryVariant(*noteTable, fileDefault);

				DrumParam param;
				param.pitch   = (*noteTable)["pitch"].value_or(69);
				param.volume  = (*noteTable)["volume"].value_or(1.0f);
				param.attack  = (*noteTable)["attack"].value_or(0.0f);
				param.hold    = (*noteTable)["hold"].value_or(0.0f);
				param.decay   = (*noteTable)["decay"].value_or(0.3f);
				param.pan     = (*noteTable)["pan"].value_or(0.5f);

				table.setDrumParam(variant, *noteNo, param);
			}

			// サブテーブル [drum.{noteNo}.{bankMSB}.*] を走査
			for(auto& [msbKey, msbValue] : *noteTable) {
				auto bankMSB = tryParseInt(msbKey.str());
				if(!bankMSB) continue;

				auto msbTable = msbValue.as_table();
				if(!msbTable) continue;

				for(auto& [lsbKey, lsbValue] : *msbTable) {
					auto bankLSB = tryParseInt(lsbKey.str());
					if(!bankLSB) continue;

					auto lsbTable = lsbValue.as_table();
					if(!lsbTable) continue;

					auto variant = readEntryVariant(*lsbTable, fileDefault);
					variant.bankMSB = static_cast<uint8_t>(*bankMSB);
					variant.bankLSB = static_cast<uint8_t>(*bankLSB);

					DrumParam param;
					param.pitch   = (*lsbTable)["pitch"].value_or(69);
					param.volume  = (*lsbTable)["volume"].value_or(1.0f);
					param.attack  = (*lsbTable)["attack"].value_or(0.0f);
					param.hold    = (*lsbTable)["hold"].value_or(0.0f);
					param.decay   = (*lsbTable)["decay"].value_or(0.3f);
					param.pan     = (*lsbTable)["pan"].value_or(0.5f);

					table.setDrumParam(variant, *noteNo, param);
				}
			}
		}
	}
}
