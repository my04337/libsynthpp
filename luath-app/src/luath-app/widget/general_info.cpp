// SPDX-FileCopyrightText: 2023 my04337
// SPDX-License-Identifier: GPL-3.0

#include <luath-app/widget/general_info.hpp>
#include <luath-app/app/application.hpp>
#include <luath-app/app/util.hpp>

using namespace luath::app::widget;
using namespace std::string_literals;
using namespace std::string_view_literals;

using lsp::synth::LuathVoice;

GeneralInfo::GeneralInfo()
{
	auto app = dynamic_cast<luath::app::Application*>(juce::JUCEApplication::getInstance());
	lsp_check(app != nullptr);

	mFont = juce::Font(app->createDefaultTypeface());
	mFont.setHeight(12);
}

GeneralInfo::~GeneralInfo()
{
}

void GeneralInfo::update(const juce::AudioDeviceManager& manager, const std::shared_ptr<const lsp::synth::LuathSynth::Digest>& digest)
{
	setParam("synth_digest"s, std::make_any<std::shared_ptr<const lsp::synth::LuathSynth::Digest>>(digest));
	setParam("audio_device_manager"s, std::make_any<const juce::AudioDeviceManager* const>(&manager));
	repaintAsync();
}

void GeneralInfo::onRendering(juce::Graphics& g, int width_, int height_, Params& params)
{
	const auto get_any_or = [&params]<class value_type>(std::string_view key, value_type && value)
	{
		return lsp::get_any_or(params, key, std::forward<value_type>(value));
	};
	auto sharedDigest = get_any_or("synth_digest"s, std::shared_ptr<const lsp::synth::LuathSynth::Digest> {});
	if(!sharedDigest) return;
	auto audioDeviceManager = get_any_or("audio_device_manager"s, static_cast<const juce::AudioDeviceManager*>(nullptr));
	if(!audioDeviceManager) return;


	auto& synthDigest = *sharedDigest;
	auto& channelDigests = synthDigest.channels;
	auto& voiceDigests = synthDigest.voices;

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
	int polyCount = 0;
	for(auto& vd : voiceDigests) {
		if(vd.ch < 1 || vd.ch > 16) continue;
		if(vd.state == LuathVoice::EnvelopeState::Free) continue;
		++polyCount;
	}

	// 背景塗りつぶし
	g.fillAll(juce::Colour::fromFloatRGBA(1.f, 1.f, 1.f, 1.f));

	// 演奏情報
	{
		auto systemType = synthDigest.systemType.toPrintableWString();
		if(auto audioDevice = audioDeviceManager->getCurrentAudioDevice()) {
			auto sampleFreq = audioDevice->getCurrentSampleRate();
			auto buffered = audioDevice->getCurrentBufferSizeSamples() / sampleFreq;
			auto latency = audioDevice->getOutputLatencyInSamples() / sampleFreq;

			drawText(0, 0, std::format(L"バッファ : {:04}[msec]", buffered * 1000));
			drawText(150, 0, std::format(L"レイテンシ : {:04}[msec]", latency * 1000));
			drawText(300, 0, std::format(L"デバイス名 　: {}", audioDevice->getName().toWideCharPointer()));
		}

		drawText(0, 15, std::format(L"演奏負荷 : {:06.2f}[%]", 100 * audioDeviceManager->getCpuUsage()));
		drawText(150, 15, std::format(L"同時発音数 : {:03}", polyCount));
		drawText(300, 15, std::format(L"MIDIリセット : {}", systemType));
		drawText(0, 30, std::format(L"マスタ音量 : {:.3f}", synthDigest.masterVolume));
	}
}