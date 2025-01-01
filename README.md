# libsynth++

マルチプラットフォーム MIDIシンセサイザ ライブラリ  
Multiplatoform MIDI synthesizer library.

本ライブラリは主に下記のサブプロジェクトにて構成されています。


## libsynth++ および luath-core (libsynth++ディレクトリ以下)
- C++ 23を全面的に活用した信号処理フレームワーク
- 軽量でシンプルな波形テーブル音源のシンセサイザ「luath」のコア実装を提供します
- ライセンス : MIT License ( https://opensource.org/license/mit/ )
- フレームワークには JUCE ライブラリを使用
    - ただし ISC Licenseで提供される juce_core, juce_audio_devices, juce_audio_basics, juce_events のみを使用
- マルチプラットフォーム対応
- CMakeプロジェクト
    - 現状はMSVC 2022環境でのみビルドを確認しています
    
## luath-app (luath-appディレクトリ以下)
- シンセサイザ「luath」を搭載した実験用アプリケーション
- ライセンス GPLv3 License ( https://opensource.org/license/gpl-3-0/ )
    - JUCEライブラリのGPLv3ライセンスに依存しています
- マルチプラットフォーム対応


## 直近のロードマップ(予定) 
- [x] v0.0.1
  - 古い実装からの一部サルベージ
- [x] v0.0.2
  - 古い実装からのサルベージ範囲拡大
- [x] v0.0.3
  - 再生処理とMIDIメッセージを独自実装からJUCEライブラリへ切替
- [x] v0.0.4
  - アプリケーションのベース部分(ウィジットツールキット等)をJUCEライブラリに置換
  - これに伴い、アプリ部分(luath-app)をGPLv3 Licenseに変更
  - シンセサイザ コア部分(libsynth++)は引き続きMIT Licenseが採用されます
- [x] v0.0.5
  - 描画処理の大幅に高速化
- [ ] v0.0.6
  - シンセサイザ部分をJUCEを用いて VST3/AU対応化する(luath-pluginとして切り出し)
  - アプリ部分(luath-app)側をJUCEを用いてVST3/AUホストにする(luath-testbedへ改名)
- [ ] v0.0.7
  - 各種プリセットや設定などをインタラクティブなUIとして構築したり保存できるようにする
- [ ] v0.0.8
  - 音色関連の拡充、演奏の品質向上
- [ ] v0.1.0
  - 全体的な体裁の調整、ドキュメント類の整備


## 補足事項
本プロジェクトは、私が最新C++を理解するためのテストベッドとして開発しているものです。

また、本プロジェクトは、私がかつて下記OSDNプロジェクトで開発していたライブラリの焼き直し版です。  
※おそらく4度目の焼き直しです。

以前のプロジェクト(ほぼ開発放棄)は下記にあります。
https://ja.osdn.net/projects/libsynthpp/