# libsynthpp

マルチプラットフォーム MIDIシンセサイザ ライブラリ  
Multiplatoform MIDI synthesizer library.

※本プロジェクトは、私がかつて下記OSDNプロジェクトで開発していたライブラリの焼き直し版です。  
https://ja.osdn.net/projects/libsynthpp/

現時点で目指しているもの :  
* 開発言語 : C++20
* 対応プラットフォーム : Windows 10
* 音声バックエンド : WASAPI (or SDL経由)
* CMakeプロジェクト(基本的にVisual Studio 2019で開発)
* 簡潔で使いやすいAPI
* 並列性が高く、レイテンシの少ない信号処理機構

MIDIシンセサイザが目指しているもの :  
* GM, GS, XGフォーマットへの対応
* 32チャネル波形テーブル音源

将来的に目指したいもの
* マルチプラットフォーム対応 (メインターゲットはWindows, Linux)
* 音声バックエンド : OpenAL
* VSTi対応
* C++20 Moduleへの対応 (CMakeの対応後)