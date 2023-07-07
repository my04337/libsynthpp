# libsynth++

マルチプラットフォーム MIDIシンセサイザ ライブラリ  
Multiplatoform MIDI synthesizer library.

※本プロジェクトは、私が最新C++を理解するためのテストベッドとして開発しているものです。
※本プロジェクトは、私がかつて下記OSDNプロジェクトで開発していたライブラリの焼き直し版です。  
https://ja.osdn.net/projects/libsynthpp/


## 信号処理ライブラリ(libsynth++)が目指しているもの

* 開発言語
    * C++23
* フレームワーク
    * JUCE ※ISC Licenseで提供される juce_core, juce_audio_devices, juce_audio_basics, juce_events のみを使用
* 対応プラットフォーム
    * マルチプラットフォーム
        * 普段の開発ではWindows 10 以降を使用
* プロジェクト構成
    * CMakeプロジェクト
* その他コンセプト
    * C++ 23の全面的な活用
    * 簡潔で使いやすいAPI
    * 将来的にC++ Modulesを利用
        * VS2022 17.6時点ではWindows.hやatlbase.hのインクルードに失敗するため見送り


## MIDIシンセサイザ(luath)部分が目指しているもの
* GM, GS, XGフォーマットへの対応
* 32チャネル波形テーブル音源

