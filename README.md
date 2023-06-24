# libsynthpp

マルチプラットフォーム MIDIシンセサイザ ライブラリ  
Multiplatoform MIDI synthesizer library.

※本プロジェクトは、私が最新C++を理解するためのテストベッドとして開発しているものです。
※本プロジェクトは、私がかつて下記OSDNプロジェクトで開発していたライブラリの焼き直し版です。  
https://ja.osdn.net/projects/libsynthpp/


## 信号処理ライブラリとして現時点で目指しているもの

* 開発言語
    * C++23
* 対応プラットフォーム
    * Windows 10 以降
* 音声バックエンド
    * WASAPI or ASIO
* 描画バックエンド
    * Direct2D, DirectWrite
* プロジェクト構成
    * CMakeプロジェクト
* その他コンセプト
    * C++ 23の全面的な活用
    * 簡潔で使いやすいAPI
    * 並列性が高く、レイテンシの少ない信号処理機構
    * 将来的にC++ Modulesを利用
        * VS2022 17.6時点ではWindows.hやatlbase.hのインクルードに失敗するため見送り


## MIDIシンセサイザ部分が目指しているもの
* GM, GS, XGフォーマットへの対応
* 32チャネル波形テーブル音源

