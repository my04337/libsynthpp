#include <lsp/midi/parser.hpp>
#include <lsp/midi/message.hpp>
#include <lsp/midi/messages/basic_message.hpp>
#include <lsp/midi/messages/extra_message.hpp>
#include <lsp/midi/messages/sysex_message.hpp>
#include <lsp/midi/messages/meta_events.hpp>
#include <lsp/base/Logging.hpp>

#include <array>
#include <fstream>

using namespace LSP;
using namespace LSP::MIDI;

// 参考URL :
//  http://eternalwindows.jp/winmm/midi/midi00.html
//  https://www.g200kg.com/jp/docs/dic/midi.html
//  https://www.cs.cmu.edu/~music/cmsip/readings/Standard-MIDI-file-format-updated.pdf

namespace 
{

// ビッグエンディアンの値を読み出します
template<
	std::integral T
>
std::optional<T> read_big(std::istream& s, size_t bytes = sizeof(T)) 
{
	// TODO リトルエンディアンでの実行前提
	// TODO アラインメントが適切ではない可能性あり
	Assertion::require(bytes <= sizeof(T));
	std::array<uint8_t, sizeof(T)> buff = {0};
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


std::pair<Header, Body> Parser::parse(const std::filesystem::path& path)
{

	std::ifstream s(path, std::ios::binary);

	if(s.fail()) {
		throw decoding_exception("invalid input");
	}
	Parser parser(s);

	// ヘッダ解析
	Header header;
	parser.stageHeader(header);

	// トラックチャンク
	std::vector<std::pair<uint64_t, std::unique_ptr<Message>>> raw_messages;
	for (uint16_t i = 0; i < header.trackNum; ++i) {
		parser.stageTrack(raw_messages);
	}
	// 時系列順にソート
	std::stable_sort(raw_messages.begin(), raw_messages.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first;});

	// 実時間に変換(マイクロ秒単位)
	Body body;
	std::chrono::microseconds pos(0); // 曲先頭からの相対時間
	std::chrono::microseconds time_per_tick(0); // 1tickあたりの時間
	
	uint64_t prev_tick = 0;
	for (auto&& [tick, msg] : raw_messages) {
		uint64_t delta = tick - prev_tick; 
		pos += time_per_tick * delta;

		if (auto meta_event = dynamic_cast<const Messages::MetaEvent*>(msg.get())) {
			// メタイベントは再生時には使用しない。 このタイミングで全て処理する
			
			// MEMO 対した数ではないので、ベタで分岐する
			if (auto ev = dynamic_cast<const Messages::SetTempo*>(meta_event)) {
				time_per_tick = ev->timePerQuarterNote() / header.ticksPerQuarterNote; 
			}

		} else {
			// その他のメッセージは実行時に処理対象とする
			body.emplace_back(pos, std::move(msg));
		}
		prev_tick = tick;
	}

	// OK
	s.close();
	return std::make_pair<Header, Body>(std::move(header), std::move(body));
}

void Parser::stageHeader(Header& header)
{
	auto require = [](const auto& optional_value) {
		if(!optional_value.has_value()) {
			throw decoding_exception("invalid header chunk");
		}
		return optional_value.value();
	};

	// チャンクタイプ
	auto chunk_type = require(read_big<uint32_t>(s));
	if(chunk_type != 0x4D546864) throw decoding_exception("invalid header chunk");

	// データ長
	auto data_length = require(read_big<uint32_t>(s));
	if(data_length < 6) throw decoding_exception("invalid header chunk : invalid length");
	std::istream::pos_type next_chunk_pos = s.tellg();
	next_chunk_pos += data_length;

	// フォーマット (フォーマット0または1のみに対応)
	auto format = require(read_big<uint16_t>(s));
	if(format >= 2) throw decoding_exception("invalid header chunk : unsupported format");

	// トラック数
	auto track_num = require(read_big<uint16_t>(s));
	
	// 時間単位
	auto time_division = require(read_big<uint16_t>(s));
	if(time_division < 0) throw decoding_exception("invalid header chunk : unsupported time division - SMPTE format");
	auto ticks_per_quarter_note = time_division;

	// ---

	// 読み終わった残りの不明なヘッダを読み飛ばす
	s.seekg(next_chunk_pos);

	// --- 

	header.format = format;
	header.trackNum = track_num;
	header.ticksPerQuarterNote = ticks_per_quarter_note;
}
void Parser::stageTrack(std::vector<std::pair<uint64_t, std::unique_ptr<Message>>>& messages)
{
	// 参考資料 : 
	//   https://www.cs.cmu.edu/~music/cmsip/readings/Standard-MIDI-file-format-updated.pdf

	auto require = [](const auto& optional_value) {
		if(!optional_value.has_value()) {
			throw decoding_exception("invalid track chunk");
		}
		return optional_value.value();
	};
	auto expect = [](auto value, auto expected) {
		if(value != expected) {
			throw decoding_exception("invalid track chunk");
		}
	};

	// チャンクタイプ
	auto chunk_type = require(read_big<uint32_t>(s));
	if(chunk_type != 0x4D54726b) throw decoding_exception("invalid track chunk");

	// データ長
	auto data_length = require(read_big<uint32_t>(s));
	std::istream::pos_type next_chunk_pos = s.tellg();
	next_chunk_pos += data_length;

	// データ
	uint8_t prev_event_state = 0x00;
	uint64_t tick_count = 0; // 現在時刻
	while (true) {
		// デルタタイム
		auto delta = require(read_variable(s));
		tick_count += delta;
		
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
			messages.emplace_back(tick_count, std::make_unique<Messages::NoteOff>(ch, note, vel));
		}	break;
		case 0x9: {	// ノートオン
			const uint8_t ch = event_state & 0x0F;
			const uint8_t note = require(read_byte(s));
			const uint8_t vel  = require(read_byte(s));
			messages.emplace_back(tick_count, std::make_unique<Messages::NoteOn>(ch, note, vel));
		}	break;
		case 0xA: {	// ポリフォニックキープレッシャー (アフタータッチ)
			const uint8_t ch = event_state & 0x0F;
			const uint8_t note = require(read_byte(s));
			const uint8_t value  = require(read_byte(s));
			messages.emplace_back(tick_count, std::make_unique<Messages::PolyphonicKeyPressure>(ch, note, value));
		}	break;
		case 0xB: {	// コントロールチェンジ
			const uint8_t ch = event_state & 0x0F;
			const uint8_t ctrl = require(read_byte(s));
			const uint8_t value  = require(read_byte(s));
			messages.emplace_back(tick_count, std::make_unique<Messages::ControlChange>(ch, ctrl, value));
		}	break;
		case 0xC: {	// プログラムチェンジ
			const uint8_t ch = event_state & 0x0F;
			const uint8_t prog = require(read_byte(s));
			messages.emplace_back(tick_count, std::make_unique<Messages::ProgramChange>(ch, prog));
		}	break;
		case 0xD: {	// チャネルプレッシャー (アフタータッチ)
			const uint8_t ch = event_state & 0x0F;
			const uint8_t value = require(read_byte(s));
			messages.emplace_back(tick_count, std::make_unique<Messages::ChannelPressure>(ch, value));
		}	break;
		case 0xE: {	// ピッチベンド
			const uint8_t ch = event_state & 0x0F;
			const uint8_t lsb = require(read_byte(s)); // 7bit
			const uint8_t msb = require(read_byte(s)); // 7bit
			const int16_t pitch = ((uint16_t(lsb)&0x7F) | ((uint16_t(msb)&0x7F) << 7)) - 8192; // 0x0000 => -8192
			messages.emplace_back(tick_count, std::make_unique<Messages::PitchBend>(ch, pitch));
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
				messages.emplace_back(tick_count, std::make_unique<Messages::SysExMessage>(std::move(sysex_event_data)));
			}	break;
			case 0x2: {	// ソングポジション
				const uint8_t lsb = require(read_byte(s)); // 7bit
				const uint8_t msb = require(read_byte(s)); // 7bit
				const int16_t pos = ((uint16_t(lsb)&0x7F) | ((uint16_t(msb)&0x7F) << 7));
				messages.emplace_back(tick_count, std::make_unique<Messages::SongPosition>(pos));
			}	break;
			case 0x3: {	// ソングセレクト
				const uint8_t song = require(read_byte(s)); 
				messages.emplace_back(tick_count, std::make_unique<Messages::SongSelect>(song));
			}	break;
			case 0x1: // 未定義(MIDIタイムコードクォーターフレーム)
			case 0x4: // 未定義
			case 0x5: // 未定義
			case 0x9: // 未定義
			case 0xD: // 未定義
				throw decoding_exception("invalid track chunk");
				break;
			case 0x6: {	// チューンリクエスト
				messages.emplace_back(tick_count, std::make_unique<Messages::TuneRequest>());
			}	break;
			case 0x8: {	// タイミングクロック
				messages.emplace_back(tick_count, std::make_unique<Messages::TimingClock>());
			}	break;
			case 0xA: {	// スタート
				messages.emplace_back(tick_count, std::make_unique<Messages::Start>());
			}	break;
			case 0xB: {	// コンティニュー
				messages.emplace_back(tick_count, std::make_unique<Messages::Continue>());
			}	break;
			case 0xC: {	// ストップ
				messages.emplace_back(tick_count, std::make_unique<Messages::Stop>());
			}	break;
			case 0xE: {	// アクティブセンシング
				messages.emplace_back(tick_count, std::make_unique<Messages::ActiveSensing>());
			}	break;
			case 0xF: {	// メタイベント
				auto meta_event_type = require(read_variable(s));
				auto meta_event_len = require(read_variable(s));
				switch (meta_event_type) {
				case 0x51:{ // Set Tempo (03 tt tt tt)
					expect(meta_event_len, 3);
					auto microseconds_per_quarter_note = std::chrono::microseconds(require(read_big<uint32_t>(s, 3)));
					messages.emplace_back(tick_count, std::make_unique<Messages::SetTempo>(microseconds_per_quarter_note));
				}	break;
				default: {
					for(uint32_t i=0; i< meta_event_len; ++i) {
						require(read_byte(s)); // skip
					}
					messages.emplace_back(tick_count, std::make_unique<Messages::GeneralMetaEvent>(meta_event_type));
				}	break;
				}
				
			}	break;
			default:
				throw decoding_exception("invalid track chunk");
			}
		}	break;
		default:
			throw decoding_exception("invalid track chunk");
		}

		// ---
		auto pos = s.tellg();
		if(pos == next_chunk_pos) break;
		if(pos > next_chunk_pos) throw decoding_exception("invalid track chunk");
		prev_event_state = event_state;
	}

	// ---

	// 次のヘッダの先頭に戻っておく
	s.seekg(next_chunk_pos);

	// --- 
}