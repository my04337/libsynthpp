#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP::MIDI::Synthesizer
{

// トーン : VCO(オシレータ), VCF(フィルタ:LPF/HPFなど), VCA(アンプ:EGなど)で構成
template<
	typename sample_type,
	class = std::enable_if_t<is_sample_type_v<sample_type>>
>
class Tone
	: non_copy_move
{
public:
	// コンストラクタ : ノートオン
	Tone() {}

	virtual ~Tone() {}

	// ノートオフ
	virtual void noteOff() = 0;

	// 音生成
	virtual sample_type update() = 0;

	// 出音中か否か
	virtual bool isBusy()const noexcept = 0;

};

}