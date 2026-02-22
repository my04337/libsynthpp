# エンベロープジェネレータ (EG) 設計資料

本ドキュメントでは、libsynthpp のエンベロープジェネレータの設計と、
エンベロープに関連する MIDI コントロールチェンジ・NRPN についてまとめます。

---

## 1. エンベロープモデル概要

本シンセサイザでは、メロディパートとドラムパートで**別々のエンベロープジェネレータクラス**を使用しています。

| クラス | モデル | 用途 |
|--------|--------|------|
| `MelodyEnvelopeGenerator` | AHDSFR | メロディパート |
| `DrumEnvelopeGenerator` | AHD | ドラムパート |

### 1.1 メロディパート用 (AHDSFR) — `MelodyEnvelopeGenerator`

```
Level
1.0 ┤     ┌──Hold──┐
    │    /          \
    │   / Attack     \ Decay
    │  /              \___Sustain___  Fade
    │ /                             \___
    │/                                  \  Release
0.0 ┤                                    └───→ Free
    └──────────────────────────────────────── Time
        noteOn                          noteOff
```

| フェーズ | 状態名 | 説明 |
|----------|--------|------|
| **Attack** | `EnvelopeState::Attack` | ノートオン直後、音量を 0 → 1.0 へ上昇させる |
| **Hold** | `EnvelopeState::Hold` | アタック完了後、最大音量 (1.0) を維持する |
| **Decay** | `EnvelopeState::Decay` | ホールド後、サスティンレベルまで音量を下降させる |
| **Fade** | `EnvelopeState::Fade` | サスティンレベルに到達後、フェードスロープに従い緩やかに減衰する |
| **Release** | `EnvelopeState::Release` | ノートオフ後、現在の音量 → 0 へ下降させる |
| **Free** | `EnvelopeState::Free` | 止音状態。ボイスは破棄対象となる |

#### パラメータ一覧 (メロディ)

| パラメータ | 単位 | 説明 |
|-----------|------|------|
| `attack_time` | 秒 | Attack フェーズの長さ |
| `hold_time` | 秒 | Hold フェーズの長さ |
| `decay_time` | 秒 | Decay フェーズの長さ |
| `sustain_level` | 0.0〜1.0 | Decay 完了後のレベル |
| `fade_slope` | level/sec or dBFS/sec | Fade の減衰率 (カーブ形状に依存) |
| `release_time` | 秒 | Release フェーズの長さ |
| `threshold_level` | 0.0〜1.0 | この値以下になると即時止音 (デフォルト 0.01) |

### 1.2 ドラムパート用 (AHD) — `DrumEnvelopeGenerator`

```
Level
1.0 ┤     ┌──Hold──┐
    │    /          \
    │   / Attack     \ Decay
    │  /              \
    │ /                \  ← threshold_level 以下で即止音
0.0 ┤                   └───→ Free
    └──────────────────────── Time
        noteOn         (noteOff無視)
```

ドラムパートではノートオフを無視します。Release/Sustain/Fade は使用しません。

#### Decay 中の毎サンプル threshold チェック

`DrumEnvelopeGenerator` では **Decay フェーズ中に毎サンプル** `threshold_level` チェックを行います。
これにより、CC 75 によるスケーリングで Decay 時間が延長された場合でも、
エンベロープレベルが閾値以下になった時点で即座に止音されます。

`MelodyEnvelopeGenerator` の Decay フェーズでは threshold チェックを行いません
（Decay 完了後に Fade フェーズが threshold チェックを担当するため）。

### 1.3 カーブ形状

| Shape | 説明 | 用途例 |
|-------|------|--------|
| **Linear** | 線形補間 | 管楽器など |
| **Exp** | RL 回路ステップ応答型の指数関数補間 | 打楽器、ピアノなど |

現在の実装では Exp カーブ (n=3.0) を標準で使用しています。
カーブ定義は `EnvelopeCurve<T>` 構造体として両クラスで共有しています。

---

## 2. コントロールチェンジ (CC) による EG 制御

### 2.1 対応 CC 一覧

| CC# | 名称 | 対象 | リアルタイム反映 | 説明 |
|-----|------|------|:---:|------|
| **72** | Release Time | メロディ | ✅ | リリースタイムのスケーリング係数を変更 |
| **73** | Attack Time | メロディ / ドラム | ❌ (生成時のみ) | アタックタイムのスケーリング係数を変更 |
| **75** | Decay Time | メロディ / ドラム | ❌ (生成時のみ) | ディケイタイムのスケーリング係数を変更 |

### 2.2 スケーリング計算式

CC 72/73/75 はいずれも同一の対数スケーリング関数で変換されます。

```
scale = 10 ^ ((ccValue / 128 - 0.5) × 4.556)
```

`MidiChannel::calcEGTimeScale(uint8_t ccValue)` として実装されています。

| CC 値 | スケール係数 | 意味 |
|:---:|:---:|------|
| 0 | ≈ 0.006 | 約 1/170 倍 (極端に短縮) |
| 32 | ≈ 0.075 | 約 1/13 倍 |
| **64** | **1.0** | **等倍 (デフォルト、変化なし)** |
| 96 | ≈ 13.3 | 約 13 倍 |
| 127 | ≈ 190 | 約 190 倍 (極端に延長) |

### 2.3 適用範囲

CC 72/73/75 は**全システムタイプ** (GM1, GM2, GS, XG) で適用されます。
デフォルト値 64 ではスケール係数が 1.0 となるため、CC 未受信時は動作に影響しません。

#### ドラム用スケール制限

ドラムパートでは CC 73/75 のスケーリング係数に**上限 4.0** を設けています。
これは、シンバル類 (d=1.5〜2.0秒) など元々長い Decay を持つ楽器において、
大きなスケーリングが適用されると異常に長い発音 (数十〜数百秒) が生じるためです。

| CC 75 | メロディ | ドラム (上限あり) |
|:---:|:---:|:---:|
| 64 | ×1.0 | ×1.0 |
| 80 | ×3.1 | ×3.1 |
| 96 | ×13.3 | **×4.0** (制限) |
| 127 | ×190 | **×4.0** (制限) |

### 2.4 リアルタイム反映 (CC 72)

CC 72 (Release Time) は発音中のメロディボイスにリアルタイムで反映されます。

- **ボイス生成時**: 楽器定義のベースリリースタイムを `Voice::mBaseReleaseTimeSec` に保存
- **CC 72 変更時**: `MidiChannel::updateReleaseTime()` → `Voice::setReleaseTimeScale()` → `MelodyEnvelopeGenerator::setReleaseTime()`
- **Release フェーズ中の更新**: 進行度 (mTime / mReleaseTime) を維持して新しい時間に補正し、エンベロープレベルの不連続を防止

CC 73 (Attack) / CC 75 (Decay) はボイス生成時にのみ適用されます。
Attack/Decay は発音直後に通過する短いフェーズであるため、リアルタイム更新の必要性は低いと判断しています。

---

## 3. EG 関連のペダル制御

### 3.1 ダンパーペダル (CC 64: Hold1)

ペダル ON 中に受信した全てのノートオフをリリース保留 (`mPendingNoteOff`) し、
ペダル OFF 時に一斉リリースします。チャネル全体に作用します。

### 3.2 ソステヌートペダル (CC 66: Sostenuto)

ペダルを踏んだ**瞬間に打鍵中のボイスのみ**をソステヌート対象とし、
対象ボイスがノートオフを受信した場合にリリースを保留します。
ペダル ON 後に新たに打鍵された音は保持対象外です。

#### Hold と Sostenuto の相互作用

| Hold | Sostenuto | ノートオフ受信時 |
|:---:|:---:|------|
| OFF | OFF | 即座にリリース |
| **ON** | OFF | リリース保留 |
| OFF | **ON** (対象ボイス) | リリース保留 |
| **ON** | **ON** | リリース保留 (両方解除されるまで保持) |

#### ソステヌート対象判定

`Voice::isNoteOn()` メソッドで正確に判定しています:

```
キー押下中 = !mPendingNoteOff && EG が Release/Free でない
```

これにより以下のケースを正しく区別します:

| ボイスの状態 | `mPendingNoteOff` | EG 状態 | `isNoteOn()` |
|---|:---:|---|:---:|
| キー押下中 | false | Attack/Hold/Decay/Fade | **true** |
| Hold/Sostenuto で保持中 | true | Attack/Hold/Decay/Fade | false |
| リリース中 | false | Release | false |
| 止音済 | false | Free | false |

---

## 4. EG 関連 NRPN (GS/XG)

### 4.1 パート共通 NRPN

| NRPN (MSB, LSB) | 名称 | 範囲 | 中心値 | 対象 | 説明 |
|:---:|------|:---:|:---:|------|------|
| (1, 99) | EG Attack Time | 0-127 | 64 | メロディ | アタックタイム微調整 (CC 73 と乗算) |
| (1, 100) | EG Decay Time | 0-127 | 64 | メロディ | ディケイタイム微調整 (CC 75 と乗算) |
| (1, 102) | EG Release Time | 0-127 | 64 | メロディ | リリースタイム微調整 (CC 72 と乗算、リアルタイム反映) |

NRPN (1, 99/100/102) は CC 72/73/75 と同一のスケーリング関数 (`calcEGTimeScale`) を使用し、
CC のスケール値と**乗算**して適用されます。

```
最終スケール = calcEGTimeScale(CC値) × calcEGTimeScale(NRPN値)
```

---

## 5. EG パラメータの適用フロー

### 5.1 メロディボイス生成時

`sMelodyParams[progId]` からベース AHDSFR パラメータ (a, h, d, s, f, r) を取得し、
以下のスケーリングを適用します。

| パラメータ | 計算式 | NRPN (GS/XG のみ) |
|-----------|--------|-------------------|
| Attack | `a × calcEGTimeScale(CC73)` | `× calcEGTimeScale(NRPN(1,99))` |
| Decay | `d × calcEGTimeScale(CC75)` | `× calcEGTimeScale(NRPN(1,100))` |
| Release | `r × calcReleaseTimeScale()` | CC 72 と NRPN(1,102) を内部でまとめて計算 |

CC 73/75/72 のスケーリングは全システムタイプで適用されます。NRPN は GS/XG のみです。

スケーリング後、以下を行います:
1. `MelodyWaveTableVoice` を生成
2. Voice に `baseReleaseTime` (スケーリング前の `r`) を保存
3. `MelodyEnvelopeGenerator::setEnvelope()` にスケーリング後の最終値を設定

### 5.2 ドラムボイス生成時

`sDrumParams[noteNo]` からベース AHD パラメータ (a, h, d) を取得し、
以下のスケーリングを適用します。

| パラメータ | 計算式 | 備考 |
|-----------|--------|------|
| Attack | `a × min(calcEGTimeScale(CC73), 4.0)` | スケール上限 4.0 |
| Decay | `d × min(calcEGTimeScale(CC75), 4.0)` | スケール上限 4.0 |

ドラムは AHD モデルのため、Release Time (CC 72) は適用されません。

スケーリング後、以下を行います:
1. `DrumWaveTableVoice` を生成
2. `DrumEnvelopeGenerator::setEnvelope()` にスケーリング後の最終値を設定

### 5.3 CC 72 リアルタイム更新フロー

CC 72 の受信、または NRPN (1, 102) の変更をトリガーとして、以下の手順で発音中のメロディボイスに反映します。

| 順序 | 処理 | 説明 |
|:---:|------|------|
| 1 | `MidiChannel::updateReleaseTime()` | CC 72 / NRPN (1,102) 変更時に呼び出し |
| 2 | `calcReleaseTimeScale()` | `calcEGTimeScale(CC72) × calcEGTimeScale(NRPN(1,102))` でスケールを算出 |
| 3 | `Voice::setReleaseTimeScale(scale)` | 各ボイスに対し `baseReleaseTime × scale` を計算 |
| 4 | `MelodyEnvelopeGenerator::setReleaseTime()` | EG のリリースタイムを更新。Release 中の場合は進行度を維持して時間を補正 |

※ `DrumWaveTableVoice` は `setReleaseTimeScale()` をオーバーライドしないため、
  ドラムボイスに対しては何も行いません (基底クラスのデフォルト実装 = no-op)。

---

## 6. ボイスクラス階層

```
Voice (基底, 抽象クラス)
├── MelodyWaveTableVoice    (メロディパート用)
│   └── MelodyEnvelopeGenerator を保持
│       setReleaseTimeScale() → EG のリリースタイムを動的更新
│       onNoteOff() → EG を Release フェーズへ遷移
│
└── DrumWaveTableVoice      (ドラムパート用)
    └── DrumEnvelopeGenerator を保持
        setReleaseTimeScale() → no-op (リリースなし)
        onNoteOff() → no-op (ノートオフ無視)
```

### Voice 基底クラスの仮想関数

| メソッド | 説明 |
|---------|------|
| `envelope()` | 現在のエンベロープレベルを返す |
| `envelopeState()` | 現在の EG 状態を返す |
| `isBusy()` | EG が稼働中 (Free 以外) か返す |
| `setReleaseTimeScale()` | リリースタイムスケーリング (メロディのみ実装) |
| `onNoteOff()` | ノートオフ時の EG 操作 (メロディ: Release 遷移, ドラム: no-op) |
| `onNoteCut()` | 強制止音時の EG リセット |

---

## 7. 関連ソースファイル

| ファイル | 内容 |
|---------|------|
| `lsp/dsp/envelope_generator.hpp` | `MelodyEnvelopeGenerator`, `DrumEnvelopeGenerator`, `EnvelopeCurve`, `EnvelopeState` |
| `lsp/synth/voice.hpp` / `.cpp` | `Voice` 基底クラス, `MelodyWaveTableVoice` (メロディ), `DrumWaveTableVoice` (ドラム) |
| `lsp/synth/midi_channel.hpp` / `.cpp` | `MidiChannel` (CC 処理、NRPN 管理、EG パラメータ伝播) |
| `lsp/synth/midi_channel_melody.cpp` | `createMelodyVoice()` (メロディ用 EG パラメータ設定) |
| `lsp/synth/midi_channel_drum.cpp` | `createDrumVoice()` (ドラム用 EG パラメータ設定) |

---

## 8. 将来の課題

- **CC 73 / CC 75 のリアルタイム反映**: 現在はボイス生成時のみ適用。Attack/Decay は短いフェーズのため優先度は低い
- **エンベロープカーブの楽器別カスタマイズ**: 現在は全楽器 Exp(n=3.0) を使用。楽器特性に応じた個別設定の余地がある

---

## 9. 参考資料

- [MIDI 規格 技術解説 (g200kg)](https://www.g200kg.com/jp/docs/tech/midi.html)
- [MIDI 1.0 Control Change Messages (midi.org)](https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2)
- [Summary of MIDI 1.0 Messages (midi.org)](https://www.midi.org/specifications-old/item/table-1-summary-of-midi-message)
