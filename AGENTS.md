# AGENTS.md - libsynthpp

コーディング規約は `.github/copilot-instructions.md` を参照。

## ビルド

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

CMake 3.26+ / Ninja / MSVC / C++23 / UNICODE・_UNICODE 有効

## アーキテクチャ

```
libsynthpp/  ← 静的ライブラリ (lsp::)   詳細は libsynthpp/AGENTS.md
luath/       ← GUIアプリ (luath::)       詳細は luath/AGENTS.md
```

- `luath` → `libsynth++` → Win32 API + 標準ライブラリのみ (外部依存なし)
- `GLOB_RECURSE` でソース収集。ファイル追加時にCMakeLists.txt編集は不要
