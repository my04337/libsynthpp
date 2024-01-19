/**
	luath-app

	Copyright(c) 2023 my04337

	This software is released under the GPLv3 License.
	https://opensource.org/license/gpl-3-0/
*/

#include <luath-app/widget/voice_info.hpp>
#include <luath-app/app/application.hpp>
#include <luath-app/app/util.hpp>

using namespace luath::app::widget;
using namespace std::string_literals;
using namespace std::string_view_literals;

using lsp::synth::LuathVoice;

VoiceInfo::VoiceInfo()
{
	auto app = dynamic_cast<luath::app::Application*>(juce::JUCEApplication::getInstance());
	check(app != nullptr);

	mFont = juce::Font(app->createDefaultTypeface());
	mFont.setHeight(12);

	mSmallFont = juce::Font(app->createDefaultTypeface());
	mSmallFont.setHeight(10);
}

VoiceInfo::~VoiceInfo()
{
}

void VoiceInfo::update(const std::shared_ptr<const lsp::synth::LuathSynth::Digest>& digest)
{
	setParam("synth_digest"s, std::make_any<std::shared_ptr<const lsp::synth::LuathSynth::Digest>>(digest));
	repaintAsync();
}

void VoiceInfo::onRendering(juce::Graphics& g, int width_, int height_, Params& params)
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
	auto drawSmallText = [&](float x, float y, std::wstring_view str) {
		g.setColour(juce::Colours::black);
		g.setFont(mSmallFont);
		g.drawSingleLineText(
			juce::String(str.data(), str.size()),
			static_cast<int>(x),
			static_cast<int>(y + mSmallFont.getAscent())
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

	// ボイス情報描画開始
	const size_t maxRowCount = 3;
	const size_t voicePerRow = 45;
	const float heightPerVoice = 12;
	check(height_ == heightPerVoice * voicePerRow);

	constexpr std::array<float, 4> columnWidth{ 15, 22, 30, 40 }; // 214
	check(width_ == static_cast<int>(std::round(std::accumulate(columnWidth.begin(), columnWidth.end(), 0.0f))* maxRowCount));

	float x = 0;
	size_t ci = 0;
	auto col = [&] {
		float ret = x;
		x += columnWidth[ci++];
		return ret;
		};
	constexpr float unscaled_width = std::accumulate(columnWidth.begin(), columnWidth.end(), 0.f);
	const float width = unscaled_width;
	{
		drawText(col(), 0, L"Ch");
		drawText(col(), 0, L"Sc.");
		drawText(col(), 0, L"Env.");
		drawText(col(), 0, L"Status");
	}
	for(size_t i = 0; i < voiceDigests.size(); ++i) {
		const auto& vd = voiceDigests[i];
		if(vd.state == LuathVoice::EnvelopeState::Free) continue;

		x = (static_cast<float>(i / voicePerRow)) * (width + 10);
		float y = heightPerVoice * ((i % voicePerRow) + 1);
		ci = 0;

		g.setColour(getMidiChannelColor(vd.ch).withMultipliedAlpha(std::clamp(vd.envelope * 0.4f + 0.1f, 0.f, 1.f)));
		g.fillRect(x, y, width, heightPerVoice);

		drawSmallText(col(), y, std::format(L"{:02}", vd.ch));
		drawSmallText(col(), y, freq2scale(vd.freq));
		drawSmallText(col(), y, std::format(L"{:.3f}", vd.envelope));
		drawSmallText(col(), y, state2text(vd.state));
	}
}