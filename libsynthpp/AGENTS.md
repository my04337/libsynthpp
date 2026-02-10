# libsynthpp

名前空間 `lsp::` の静的ライブラリ。

## ディレクトリ構成

- `src/lsp/core/` — 基盤 (型定義, ログ, シグナル, 数学)
- `src/lsp/dsp/` — DSP部品 (FFT, エンベロープ, フィルタ, 波形テーブル)
- `src/lsp/synth/` — シンセサイザ (Voice, MidiChannel, Synthesizer)
- `src/lsp/midi/` — MIDI (SMFパーサ, シーケンサ, メッセージ型)
- `src/lsp/audio/` — 音声出力 (WASAPI, WAVファイル)

## 主要な基盤クラス・マクロ

| 名前 | 場所 | 用途 |
|------|------|------|
| `non_copy` | core/base.hpp | コピー禁止ミックスイン |
| `non_copy_move` | core/base.hpp | コピー・ムーブ禁止ミックスイン |
| `Signal<T>` | core/signal.hpp | 多チャネル音声信号バッファ (ムーブのみ) |
| `Log` | core/logging.hpp | `std::format`ベースの静的ロガー (`Log::d(...)` 等) |
| `lsp_check(expr)` | core/logging.hpp | アサーション (失敗時 `std::unreachable`) |
| `finally(f)` | core/base.hpp | スコープガード |

## 注意事項

- `WIN32_LEAN_AND_MEAN`, `STRICT`, `NOMINMAX` は `base.hpp` で定義済み。再定義しないこと
