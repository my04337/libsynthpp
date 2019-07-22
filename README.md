# libsynthpp

マルチプラットフォーム 信号処理ライブラリ &amp;MIDIシンセサイザ サンプル  
Multiplatoform signal processing library &amp; MIDI synthesizer sample.

※本プロジェクトは、私がかつて下記OSDNプロジェクトで開発していたライブラリの焼き直し版です。  
https://ja.osdn.net/projects/libsynthpp/

現時点で目指しているもの :  
* 開発言語 : C++17
* 対応プラットフォーム : Windows 7以降
* 音声バックエンド : WASAPI
* CMakeプロジェクト(Visual Studio 2017上で開発)
* 簡潔で使いやすいAPI
* 並列性が高く、レイテンシの少ない信号処理機構

MIDIシンセサイザ(ライブラリの実装サンプル兼テストベッド)が目指しているもの :  
* GM, GS, XGフォーマットへの対応
* 32チャネル波形テーブル音源

将来的に目指したいもの
* マルチプラットフォーム対応 (メインターゲットはWindows, Linux)
* 音声バックエンド : OpenAL
