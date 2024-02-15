/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/channel_info.hpp>
#include <luath-app/app/application.hpp>
#include <luath-app/app/util.hpp>

using namespace luath::app::widget;
using namespace std::string_literals;
using namespace std::string_view_literals;

using lsp::synth::LuathVoice;

ChannelInfo::ChannelInfo()
{
	auto app = dynamic_cast<luath::app::Application*>(juce::JUCEApplication::getInstance());
	lsp_check(app != nullptr);

	mFont = juce::Font(app->createDefaultTypeface());
	mFont.setHeight(12);
}

ChannelInfo::~ChannelInfo()
{
}

void ChannelInfo::update(const std::shared_ptr<const lsp::synth::LuathSynth::Digest>& digest)
{
	setParam("synth_digest"s, std::make_any<std::shared_ptr<const lsp::synth::LuathSynth::Digest>>(digest));
	repaintAsync();
}

void ChannelInfo::onRendering(juce::Graphics& g, int width_, int height_, Params& params)
{
	// 描画準備
	const auto get_any_or = [&params]<class value_type>(std::string_view key, value_type && value)
	{
		return lsp::get_any_or(params, key, std::forward<value_type>(value));
	};
	auto sharedDigest = get_any_or("synth_digest"s, std::shared_ptr<const lsp::synth::LuathSynth::Digest> {});
	if(!sharedDigest) return;
	auto& channelDigests = sharedDigest->channels;
	auto& voiceDigests = sharedDigest->voices;

	auto drawText = [&](float x, float y, std::wstring_view str) {
		g.setColour(juce::Colours::black);
		g.setFont(mFont);
		g.drawSingleLineText(
			juce::String(str.data(), str.size()),
			static_cast<int>(x),
			static_cast<int>(y + mFont.getAscent())
		);
	};

	// チャネル毎の合計発音数を取得
	std::array<int, 16> poly;
	std::fill(poly.begin(), poly.end(), 0);
	for(auto& vd : voiceDigests) {
		if(vd.ch < 1 || vd.ch > 16) continue;
		if(vd.state == LuathVoice::EnvelopeState::Free) continue;
		++poly[vd.ch - 1];
	}

	// 背景塗りつぶし
	g.fillAll(juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 1.f));

	// チャネル情報描画
	const float width = static_cast<float>(width_);
	const float heightPerChannel = 15;
	lsp_check(height_ == heightPerChannel * (channelDigests.size() + 1));

	constexpr std::array<float, 10> columnWidth{ 25, 75, 40, 40, 60, 35, 75, 25, 25, 35 }; // 435
	lsp_check(width_ == static_cast<int>(std::round(std::accumulate(columnWidth.begin(), columnWidth.end(), 0.0f))));

	float x = 0;
	size_t ci = 0;
	auto col = [&] {
		float ret = x;
		x += columnWidth[ci++];
		return ret;
	};
	float y = 0;
	{
		x = 0;
		ci = 0;
		drawText(col(), y, L"Ch");
		drawText(col(), y, L"Ins");
		drawText(col(), y, L"Vol");
		drawText(col(), y, L"Exp");
		drawText(col(), y, L"Pitch");
		drawText(col(), y, L"Pan");
		drawText(col(), y, L"Envelope");
		drawText(col(), y, L"Ped");
		drawText(col(), y, L"Drm");
		drawText(col(), y, L"Poly");
	}
	for(const auto& cd : channelDigests) {
		y += heightPerChannel;
		x = 0;
		ci = 0;

		g.setColour(getMidiChannelColor(cd.ch).withMultipliedAlpha(0.5f));
		g.fillRect(x, y, width, heightPerChannel);

		drawText(col(), y, std::format(L"{:02}", cd.ch));
		drawText(col(), y, std::format(L"{:03}:{:03}.{:03}", cd.progId, cd.bankSelectMSB, cd.bankSelectLSB));
		drawText(col(), y, std::format(L"{:0.3f}", cd.volume));
		drawText(col(), y, std::format(L"{:0.3f}", cd.expression));
		drawText(col(), y, std::format(L"{:+0.4f}", cd.pitchBend));
		drawText(col(), y, std::format(L"{:0.2f}", cd.pan));
		drawText(col(), y, std::format(L"{:03}.{:03}.{:03}",
			static_cast<int>(std::round(cd.attackTime * 127)),
			static_cast<int>(std::round(cd.decayTime * 127)),
			static_cast<int>(std::round(cd.releaseTime * 127))));
		drawText(col(), y, cd.pedal ? L"on" : L"off");
		drawText(col(), y, cd.drum ? L"on" : L"off");
		drawText(col(), y, std::format(L"{:02}", poly[cd.ch - 1]));
	}
}