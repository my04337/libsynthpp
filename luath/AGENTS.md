# luath

名前空間 `luath::` のGUIアプリケーション。`libsynth++` に依存。

- `target_link_libraries` で `libsynth++` をリンク。`#include <lsp/...>` で参照可能

## ディレクトリ構成

- `src/luath/core/` — `lsp` 名前空間のusing + D2D/DWriteヘッダ集約
- `src/luath/app/` — WinMainエントリポイント, Application
- `src/luath/window/` — メインウィンドウ (Win32 WndProc)
- `src/luath/widget/` — 描画ウィジェット (オシロスコープ, スペアナ, リサージュ)
- `src/luath/drawing/` — Direct2D描画補助 (FontLoader等)

## 注意事項

- `Synthesizer` のレンダリングコールバックは別スレッドから呼ばれる。共有状態は `std::atomic` または排他制御を使用すること
