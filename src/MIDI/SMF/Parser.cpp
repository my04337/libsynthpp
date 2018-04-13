#include <LSP/MIDI/SMF/Parser.hpp>
#include <LSP/Base/Logging.hpp>
#include <array>

using namespace LSP;
using namespace LSP::MIDI;
using namespace LSP::MIDI::SMF;

// 参考URL :
//  http://eternalwindows.jp/winmm/midi/midi00.html
//  https://www.g200kg.com/jp/docs/dic/midi.html
//  http://www.music.mcgill.ca/~ich/classes/mumt306/midiformat.pdf

namespace 
{

// ビッグエンディアンの値を読み出します
template<
	typename T,
	class = std::enable_if_t<std::is_arithmetic_v<T>>
>
std::optional<T> read_big(std::istream& s) 
{
	// TODO リトルエンディアンでの実行前提
	// TODO アラインメントが適切ではない可能性あり
	constexpr size_t bytes = sizeof(T);
	std::array<uint8_t, bytes> buff;
	for (size_t i = 0; i < bytes; ++i) {
		int b = s.get();
		if(b < 0) return {};
		buff[bytes-i-1] = static_cast<uint8_t>(b);
	}
	return *reinterpret_cast<T*>(&buff[0]);
}

// 1バイトを読み出します
std::optional<uint8_t> read_byte(std::istream& s) 
{
	// TODO リトルエンディアンでの実行前提
	// TODO アラインメントが適切ではない可能性あり

	int b = s.get();
	if(b < 0) return {};
	return static_cast<uint8_t>(b);
}

// デルタタイム(可変長数値)を読み出します
std::optional<uint32_t> read_variable(std::istream& s) 
{
	uint32_t ret = 0;
	int read_bytes = 0;
	while (true) {
		// 最大でも4バイトで構成される
		if(read_bytes >= 4) return {}; 

		int b = s.get();
		++read_bytes;
		if(b < 0) return {}; // EOF or error

		// 数値として使う値は7bit単位で格納されている
		ret <<= 7;
		ret += static_cast<uint8_t>(b & 0x7F);

		// 最上位桁が0ならば、この桁で終わり
		if((b & 0x80) == 0) break;
	}
	return ret;
}

}


Parser::Parser()
{
}

void Parser::parse(std::istream& s)
{
	if(s.fail()) {
		throw parsing_exception("invalid input");
	}


	// ヘッダ解析
	stageHeader(s);

	// トラックチャンク
	for (uint16_t i = 0; i < mHeader.trackNum; ++i) {
		stageTrack(s);
	}

	// OK
}

void Parser::stageHeader(std::istream& s)
{
	auto require = [](const auto& optional_value) {
		if(!optional_value.has_value()) {
			throw parsing_exception("invalid header chunk");
		}
		return optional_value.value();
	};

	// チャンクタイプ
	auto chunk_type = require(read_big<uint32_t>(s));
	if(chunk_type != 0x4D546864) throw parsing_exception("invalid header chunk");

	// データ長
	auto data_length = require(read_big<uint32_t>(s));
	if(data_length < 6) throw parsing_exception("invalid header chunk : invalid length");
	std::istream::pos_type next_chunk_pos = s.tellg();
	next_chunk_pos += data_length;

	// フォーマット (フォーマット0または1のみに対応)
	auto format = require(read_big<uint16_t>(s));
	if(format >= 2) throw parsing_exception("invalid header chunk : unsupported format");

	// トラック数
	auto track_num = require(read_big<uint16_t>(s));
	
	// 時間単位
	auto time_division = require(read_big<uint16_t>(s));
	if(time_division < 0) throw parsing_exception("invalid header chunk : unsupported time division - SMPTE format");
	auto ticks_per_frame = time_division;

	// ---

	// 読み終わった残りの不明なヘッダを読み飛ばす
	s.seekg(next_chunk_pos);

	// --- 

	mHeader.format = format;
	mHeader.trackNum = track_num;
	mHeader.ticksPerFrame = ticks_per_frame;
}
void Parser::stageTrack(std::istream& s)
{
	auto require = [](const auto& optional_value) {
		if(!optional_value.has_value()) {
			throw parsing_exception("invalid track chunk");
		}
		return optional_value.value();
	};

	// チャンクタイプ
	auto chunk_type = require(read_big<uint32_t>(s));
	if(chunk_type != 0x4D54726b) throw parsing_exception("invalid track chunk");

	// データ長
	auto data_length = require(read_big<uint32_t>(s));
	std::istream::pos_type next_chunk_pos = s.tellg();
	next_chunk_pos += data_length;

	// データ
	uint8_t prev_event_state = 0x00;
	while (true) {
		// デルタタイム
		auto delta = require(read_variable(s));
		
		// イベント
		auto event_state = require(read_byte(s)); // ステータスバイト
		if ((event_state & 0x80) == 0) {
			// ランニングステータス
			event_state = prev_event_state;
			s.seekg(-1, std::ios::cur);
		}
		switch ((event_state & 0xF0) >> 4) {
		case 0x8: {	// ノートオフ
			const uint8_t ch = event_state & 0x0F;
			const uint8_t note = require(read_byte(s));
			const uint8_t vel  = require(read_byte(s));
		}	break;
		case 0x9: {	// ノートオン
			const uint8_t ch = event_state & 0x0F;
			const uint8_t note = require(read_byte(s));
			const uint8_t vel  = require(read_byte(s));
		}	break;
		case 0xA: {	// アフタータッチ (ポリフォニックキープレッシャー)
			const uint8_t ch = event_state & 0x0F;
			const uint8_t note = require(read_byte(s));
			const uint8_t value  = require(read_byte(s));
		}	break;
		case 0xB: {	// コントロールチェンジ
			const uint8_t ch = event_state & 0x0F;
			const uint8_t ctrl = require(read_byte(s));
			const uint8_t value  = require(read_byte(s));
		}	break;
		case 0xC: {	// コントロールチェンジ
			const uint8_t ch = event_state & 0x0F;
			const uint8_t prog = require(read_byte(s));
		}	break;
		case 0xD: {	// アフタータッチ (チャネルプレッシャー)
			const uint8_t ch = event_state & 0x0F;
			const uint8_t value = require(read_byte(s));
		}	break;
		case 0xE: {	// ピッチベンド
			const uint8_t ch = event_state & 0x0F;
			const uint8_t lsb = require(read_byte(s)); // 7bit
			const uint8_t msb = require(read_byte(s)); // 7bit
		}	break;
		case 0xF: {
			switch (event_state & 0x0F) {
			case 0x0:	// システムエクスクルーシブ (F0 <len> <data...> 形式, data末尾はF7)
			case 0x7:{	// システムエクスクルーシブ	(F7 <len> <data...> 形式)
				auto sysex_event_len = require(read_variable(s));
				std::vector<uint8_t> sysex_event_data;
				for(uint32_t i=0; i< sysex_event_len; ++i) {
					auto b = require(read_byte(s));
					sysex_event_data.push_back(b);
				}
			}	break;
			case 0x1:{	// MIDIタイムコードクォーターフレーム
				const uint8_t time_code = require(read_byte(s));
			}	break;
			case 0x2: {	// ソングポジション
				const uint8_t lsb = require(read_byte(s)); // 7bit
				const uint8_t msb = require(read_byte(s)); // 7bit
			}	break;
			case 0x3: {	// ソングセレクト
				const uint8_t song = require(read_byte(s)); 
			}	break;
			case 0x4: // 未定義
			case 0x5: // 未定義
			case 0x9: // 未定義
			case 0xD: // 未定義
				throw parsing_exception("invalid track chunk");
				break;
			case 0x6: {	// チューンリクエスト
			}	break;
			case 0x8: {	// タイミングクロック
			}	break;
			case 0xA: {	// スタート
			}	break;
			case 0xB: {	// コンティニュー
			}	break;
			case 0xC: {	// ストップ
			}	break;
			case 0xE: {	// アクティブセンシング
			}	break;
			case 0xF: {	// メタイベント
				auto meta_event_type = require(read_variable(s));
				auto meta_event_len = require(read_variable(s));
				std::vector<uint8_t> meta_event_data;
				for(uint32_t i=0; i< meta_event_len; ++i) {
					auto b = require(read_byte(s));
					meta_event_data.push_back(b);
				}
			}	break;
			default:
				throw parsing_exception("invalid track chunk");
			}
		}	break;
		default:
			throw parsing_exception("invalid track chunk");
		}

		// ---
		auto pos = s.tellg();
		if(pos == next_chunk_pos) break;
		if(pos > next_chunk_pos) throw parsing_exception("invalid track chunk");
		prev_event_state = event_state;
	}

	// ---

	// 次のヘッダの先頭に戻っておく
	s.seekg(next_chunk_pos);

	// --- 
}