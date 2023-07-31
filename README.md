# libsynth++

マルチプラットフォーム MIDIシンセサイザ ライブラリ  
Multiplatoform MIDI synthesizer library.

本ライブラリは主に下記のサブプロジェクトにて構成されています。

## libsynth++ および luath-core (libsynth++ディレクトリ以下)
* C++ 23を全面的に活用した信号処理フレームワーク
* 軽量でシンプルな波形テーブル音源のシンセサイザ「luath」コア実装
* ライセンス
    * MIT License 
    * https://opensource.org/license/mit/
* フレームワークには JUCE ライブラリを使用
    * ただし ISC Licenseで提供される juce_core, juce_audio_devices, juce_audio_basics, juce_events のみを使用
* マルチプラットフォーム対応
* CMakeプロジェクト
    
## luath-app および luath-plugin (luath-appおよびluath-pluginディレクトリ以下)
* シンセサイザ(LuathSynth)を搭載した実験用アプリケーション
* ライセンス
    * GPLv3 License
    * https://opensource.org/license/gpl-3-0/
* フレームワークには JUCE ライブラリを使用
    * JUCEライブラリの内、ISC LicenseおよびGPLv3ライセンスを用いている箇所を使用
    
----

なお、本プロジェクトは、私が最新C++を理解するためのテストベッドとして開発しているものです。

また、本プロジェクトは、私がかつて下記OSDNプロジェクトで開発していたライブラリの焼き直し版です。  
※おそらく4度目の焼き直しです。

https://ja.osdn.net/projects/libsynthpp/

