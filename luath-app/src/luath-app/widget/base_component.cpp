/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/base_component.hpp>

using namespace luath::app::widget;
using namespace std::string_literals;
using namespace std::string_view_literals;

BaseComponent::BaseComponent()
{
	// 自前で描画バッファを持つため、juce::Component側の描画バッファは不要
	setBufferedToImage(false);

	// 描画スレッド開始
	mRenderingThread = std::jthread([this](std::stop_token stopToken) {renderingThreadMain(stopToken); });

	// 初回の描画を予約
	mRenderingRequestEvent.set();
}

BaseComponent::~BaseComponent()
{
	// 描画スレッドを安全に停止
	if(mRenderingThread.joinable()) {
		mRenderingThread.request_stop();
		mRenderingThread.join();
	}
}

void BaseComponent::setParam(std::string_view key, std::any&& value)
{
	std::lock_guard lock(mMutexForParams);
	mParams.insert_or_assign(std::string(key), std::move(value));
}

void BaseComponent::unsetParam(std::string_view key)
{
	std::lock_guard lock(mMutexForParams);
	auto found = mParams.find(key);
	if(found == mParams.end()) return;

	mParams.erase(found);
}

void BaseComponent::repaintAsync()
{
	juce::MessageManager::callAsync([target = juce::WeakReference<Component>{ this }]
	{
		if(target != nullptr) {
			target->repaint();
		}
	});
}

void BaseComponent::paint(juce::Graphics& g)
{
	// 以前に描画した画像を出力する
	{
		std::lock_guard lock(mRenderingMutex);

		// スレッドセーフに新たな描画サイズを記録
		int width = getWidth();
		int height = getHeight();
		float scaleFactor = g.getInternalContext().getPhysicalPixelScaleFactor();
		int scaledWidth = static_cast<int>(width * scaleFactor);
		int scaledHeight = static_cast<int>(height * scaleFactor);

		mParams.insert_or_assign("width"s, std::make_any<int>(width));
		mParams.insert_or_assign("height"s, std::make_any<int>(height));
		mParams.insert_or_assign("scale_factor"s, std::make_any<float>(scaleFactor));

		// 丸め方が混在すると混乱を招くため、スケール後のサイズ(整数値)もここで定義する
		mParams.insert_or_assign("scaled_width"s, scaledWidth);
		mParams.insert_or_assign("scaled_height"s, scaledHeight);

		// 描画済画像を出力
		g.drawImage(*mRenderedImageForPaint, 0, 0, width, height, 0, 0, scaledWidth, scaledHeight);

		// 必要に応じて次の描画対象を現在の描画サイズに変更
		// MEMO この操作はUIスレッドで行う必要がある。 スレッド間を跨いだ juce::Imageのやりとりはスレッドセーフではない
		if(mRenderedImageForRendering->getWidth() != scaledWidth || mRenderedImageForRendering->getHeight() != scaledHeight) {
			*mRenderedImageForRendering = juce::Image(juce::Image::ARGB, scaledWidth, scaledHeight, true);
		}
	}
	// 新たな描画をリクエストする
	mRenderingRequestEvent.set();
}
void BaseComponent::renderingThreadMain(std::stop_token stopToken)
{
	using std::views::zip;

	// 描画用信号バッファ
	std::array<std::vector<float>, 2> drawingBuffer;

	// 描画スレッド用の描画バッファ
	juce::Image drawnImage;


	// 描画ループ 
	while(mRenderingRequestEvent.try_wait(stopToken, std::chrono::milliseconds(10))) {
		std::unique_lock lockForParams(mMutexForParams);
		Params params = mParams;
		lockForParams.unlock();

		const auto get_any_or = [&params]<class value_type>(std::string_view key, value_type && value)
		{
			return lsp::get_any_or(params, key, std::forward<value_type>(value));
		};

		const auto width = get_any_or("width"sv, 0);
		const auto height = get_any_or("height"sv, 0);
		const auto scaleFactor = get_any_or("scale_factor"sv, 1.f);

		const auto scaledWidth = get_any_or("scaled_width"sv, 0);
		const auto scaledHeight = get_any_or("scaled_height"sv, 0);

		const juce::Rectangle<int> unscaledCanvasRect = juce::Rectangle<int>{ 0, 0, width, height };
		const juce::Rectangle<int> scaledCanvasRect = juce::Rectangle<int>{ 0, 0, scaledWidth, scaledHeight };

		// 描画サイズが0ならば描画しない
		if(scaledWidth <= 0 || scaledHeight <= 0) continue;

		// 描画バッファの更新
		if(drawnImage.getWidth() != scaledCanvasRect.getWidth() || drawnImage.getHeight() != scaledCanvasRect.getHeight()) {
			drawnImage = juce::Image(juce::Image::ARGB, scaledCanvasRect.getWidth(), scaledCanvasRect.getHeight(), true);
		}
		else {
			drawnImage.clear(drawnImage.getBounds());
		}

		// 描画開始
		{
			juce::Graphics g(drawnImage);
			g.addTransform(juce::AffineTransform::scale(scaleFactor));

			if(unscaledCanvasRect.getWidth() > 0 && unscaledCanvasRect.getHeight() > 0) {
				onRendering(g, unscaledCanvasRect.getWidth(), unscaledCanvasRect.getHeight(), params);
			}
		}

		// 描画結果を描画スレッド外に安全に転送する
		{
			std::lock_guard lock(mRenderingMutex);
			if(!mRenderedImageForRendering->isNull()){
				mRenderedImageForRendering->clear(mRenderedImageForRendering->getBounds());
				juce::Graphics g(*mRenderedImageForRendering);
				g.drawImageAt(drawnImage, 0, 0);
			}
			std::swap(mRenderedImageForPaint, mRenderedImageForRendering); // flip
		}
	}
}