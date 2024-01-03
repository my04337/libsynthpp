/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/abstract_drawable_component.hpp>

using namespace luath::app::widget;
using namespace std::string_literals;
using namespace std::string_view_literals;

AbstractDrawableComponent::AbstractDrawableComponent()
{
	mDrawingThead = std::jthread([this](std::stop_token stopToken) {drawingThreadMain(stopToken); });
}

AbstractDrawableComponent::~AbstractDrawableComponent()
{
	// 描画スレッドを安全に停止
	if(mDrawingThead.joinable()) {
		mDrawingThead.request_stop();
		mDrawingThead.join();
	}
}

void AbstractDrawableComponent::setParam(std::string_view key, std::any&& value)
{
	std::lock_guard lock(mMutexForParams);
	mParams.insert_or_assign(std::string(key), std::move(value));
}

void AbstractDrawableComponent::unsetParam(std::string_view key)
{
	std::lock_guard lock(mMutexForParams);
	auto found = mParams.find(key);
	if(found == mParams.end()) return;

	mParams.erase(found);
}


void AbstractDrawableComponent::paint(juce::Graphics& g)
{
	// 以前に描画した画像を出力する
	{
		std::lock_guard lock(mDrawingMutex);

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
		g.drawImage(mDrawnImage, getX(), getY(), width, height, 0, 0, scaledWidth, scaledHeight);
	}
	// 新たな描画をリクエストする
	mRequestDrawEvent.set();
}
void AbstractDrawableComponent::drawingThreadMain(std::stop_token stopToken)
{
	using std::views::zip;

	// 描画用信号バッファ
	std::array<std::vector<float>, 2> drawingBuffer;

	// 静的な表示部分の描画キャッシュ
	juce::Image cachedStaticImage;


	// 描画ループ 
	while(mRequestDrawEvent.try_wait(stopToken, std::chrono::milliseconds(10))) {
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

		juce::Image drawnImage = juce::Image(juce::Image::ARGB, scaledCanvasRect.getWidth(), scaledCanvasRect.getHeight(), true);
		{
			juce::Graphics g(drawnImage);
			g.drawImageAt(cachedStaticImage, 0, 0); // 静的部分はスケーリングされる前に描画
			g.addTransform(juce::AffineTransform::scale(scaleFactor));

			if(unscaledCanvasRect.getWidth() > 0 && unscaledCanvasRect.getHeight() > 0) {
				onDrawElements(g, unscaledCanvasRect.getWidth(), unscaledCanvasRect.getHeight(), params);
			}
		}

		// 描画結果を転送
		{
			std::lock_guard lock(mDrawingMutex);
			mDrawnImage = std::move(drawnImage);
		}
	}
}