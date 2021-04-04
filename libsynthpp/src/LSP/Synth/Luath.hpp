#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/MIDI/Message.hpp>
#include <LSP/MIDI/MessageReceiver.hpp>
#include <LSP/Synth/VoiceMapper.hpp>
#include <LSP/Synth/Voice.hpp>
#include <LSP/Threading/TaskDispatcher.hpp>

#include <array>

namespace LSP::Synth
{

// Luath : Simple synthesizer implimentation
class Luath
	: public LSP::MIDI::MessageReceiver
{
public:
	using RenderingCallback = std::function<void(LSP::Signal<float>&& sig)>;
	static constexpr uint8_t MAX_CHANNELS = 16;
	enum class SystemType
	{
		GM1,
		GM2,
		GS,
	};
	struct Statistics {
		uint64_t created_samples = 0;
		uint64_t skipped_samples = 0;

		clock::duration cycle_time;
		clock::duration rendering_time;
		float rendering_load_average()const noexcept {
			if (cycle_time.count() > 0) {
				return (float)rendering_time.count() / (float)cycle_time.count();
			} else {
				return 0;
			}
		}
	};

public:
	Luath(uint32_t sampleFreq, SystemType defaultSystemType = SystemType::GS);
	~Luath();

	void dispose();

	// MIDIメッセージを受信した際にコールバックされます。
	virtual void onMidiMessageReceived(clock::time_point received_time, const std::shared_ptr<const LSP::MIDI::Message>& msg)override;
	
	// 音声が生成された際のコールバック関数を設定します
	void setRenderingCallback(RenderingCallback cb);

	// 統計情報を取得します
	Statistics statistics()const;

protected:
	void playingThreadMain();
	void dispatchMessage(const std::shared_ptr<const LSP::MIDI::Message>& msg);
	void reset(SystemType type);

	// ノートオン
	void noteOn(uint8_t ch, uint8_t noteNo, uint8_t vel);

	// ノートオフ
	void noteOff(uint8_t ch, uint8_t noteNo);

	// コントロールチェンジ
	void controlChange(uint8_t ch, uint8_t ctrlNo, uint8_t value);

	// システムエクスクルーシブ
	void sysExMessage(const uint8_t* data, size_t len);

	// MIDIメッセージを元に演奏した結果を返します
	LSP::Signal<float> generate(size_t len);

private:
	mutable std::mutex mMutex;
	std::pmr::synchronized_pool_resource mMem;
	std::deque<std::pair<clock::time_point, std::shared_ptr<const LSP::MIDI::Message>>> mMessageQueue;
	LSP::Threading::TaskDispatcher mTaskDispatcher;

	Statistics mStatistics;
	std::atomic<Statistics> mThreadSafeStatistics;

	RenderingCallback mRenderingCallback;
	
	// all channel parameters
	const uint32_t mSampleFreq;
	SystemType mSystemType;

	// per channel parameters

	struct PerChannelParams 
	{
		PerChannelParams(uint32_t sampleFreq, uint8_t ch);

		void reset(SystemType type);
		void resetParameterNumberState();
		// ---
		void noteOn(uint32_t noteNo, uint8_t vel);
		void noteOff(uint32_t noteNo);
		void holdOn();
		void holdOff();
		// ---
		std::pair<float,float> update();
		// ---


		// サンプリング周波数(実行時に動的にセット)
		const uint32_t sampleFreq;
		// チャネル番号(実行時に動的にセット)
		const uint8_t ch;
		
		// プログラムチェンジ
		uint8_t pcId; // プログラムId
		LSP::Filter::EnvelopeGenerator<float> pcEG; // チャネルEG(パラメータ計算済)
		void updateProgram();

		// コントロールチェンジ
		uint8_t ccPrevCtrlNo;
		uint8_t ccPrevValue;
		float ccPan;		// CC:10 - パン [0.0(左), +1.0(右)]
		float ccExpression;	// CC:11 - エクスプレッション [0.0, +1.0]

		// RPN/NRPN State
		std::optional<uint8_t> ccRPN_MSB;
		std::optional<uint8_t> ccRPN_LSB;
		std::optional<uint8_t> ccNRPN_MSB;
		std::optional<uint8_t> ccNRPN_LSB;
		std::optional<uint8_t> ccDE_MSB;
		std::optional<uint8_t> ccDE_LSB;

		// RPN
		int16_t rpnPitchBendSensitibity;
		bool    rpnNull;

	private:
		void voice_noteOn(VoiceId id, uint32_t noteNo, uint8_t vel);
		void voice_noteOff(VoiceId id);

	private:
		// 発音状態管理
		LSP::Synth::VoiceMapper _voiceMapper;
		// ボイス生成
		std::unordered_map<VoiceId, std::unique_ptr<LSP::Synth::Voice>> _voices;
	};
	std::vector<PerChannelParams> mPerChannelParams;


	// 演奏スレッド
	std::thread mPlayingThread;
	std::atomic_bool mPlayingThreadAborted;
};

}