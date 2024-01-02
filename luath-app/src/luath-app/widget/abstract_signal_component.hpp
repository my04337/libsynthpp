/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#pragma once

#include <luath-app/core/core.hpp>
#include <lsp/util/auto_reset_event.hpp>

namespace luath::app::widget
{

class AbstractSignalComponent
	: public juce::Component
{
public:
	using Extras = std::unordered_map<std::string, std::any, lsp::string_hash, std::equal_to<>>;

public:
	AbstractSignalComponent();
	~AbstractSignalComponent() override;

	// 表示波形を書き込みます
	void write(const lsp::Signal<float>& sig);

	// リサージュ曲線を描画を描画します
	void paint(juce::Graphics& g)override final;

protected:
	// バッファサイズを指定します
	void setSpan(float sampleFreq, float span);
	void setBufferSize(float sampleFreq, size_t bufferSize);
	void setExtra(std::string_view key, std::any&& value);

	// 描画サイズ以外に影響の受けない部分の描画時にコールバックされます
	virtual void onDrawStaticElements(juce::Graphics& g, int width, int height, Extras& extras) = 0;

	// 動的に変化する部分の描画時にコールバックされます
	virtual void onDrawDynamicElements(juce::Graphics& g, int width, int height, Extras& extras, std::array<std::vector<float>, 2>& buffer) = 0;

private:
	// 描画スレッドメイン
	void drawingThreadMain(std::stop_token stopToken);

private:
	// 入力用信号バッファ ※mInputMutexにて保護される
	mutable std::mutex mInputMutex;
	std::array<std::deque<float>, 2> mInputBuffer;


	// 各種パラメータ ※mInputMutexにて保護される
	// MEMO 描画の度にコピーされるため最小限のデータのみ格納すること
	Extras mExtras;
	
	// 描画スレッド
	std::jthread mDrawingThead;

	// 描画キャッシュ ※mDrawingMutexにて保護される
	mutable std::mutex mDrawingMutex;
	AutoResetEvent mRequestDrawEvent;
	juce::Image mDrawnImage;
};

} // namespace luath::app::widget