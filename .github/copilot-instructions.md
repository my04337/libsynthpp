# Copilot Instructions - libsynthpp

## コーディング規約
- 文字コード: UTF-8 (BOMなし)。ただしBOMが必要な場合はBOMを付ける
- 改行コード: CRLF
- ヘッダファイル: `.hpp`, ソースファイル: `.cpp`
- インクルードガード: `#pragma once`
- 名前空間: `lsp::` (ライブラリ), `luath::` (アプリ)
- メンバ変数プレフィックス: `m` (例: `mWindowHandle`, `mSynthesizer`)
- `constexpr`, `[[nodiscard]]`, `[[likely]]` 等のモダンC++属性を積極的に使用
- `std::format` を文字列フォーマットに使用
- COM スマートポインタ: `CComPtr<T>`
- 定数: `static constexpr` + UPPER_SNAKE_CASE
- コメントは日本語で記述
- インデント: タブ
- 新規ファイルは SPDX ヘッダ2行 + `#pragma once` で開始
  ```cpp
  // SPDX-FileCopyrightText: <作成年> my04337
  // SPDX-License-Identifier: MIT
  ```
  - 新規作成: `<作成年>` = 現在の年（例: 2026）
  - 大幅改変: 範囲表記に更新（例: 2018-2026）
  - 既存ファイルに SPDX ヘッダが無い場合、追加しない
- HRESULT は `lsp_check(SUCCEEDED(...))` で検査する
