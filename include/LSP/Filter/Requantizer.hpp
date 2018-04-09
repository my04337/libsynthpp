#pragma once

#include <LSP/Base/Base.hpp>
#include <LSP/Base/Signal.hpp>
#include <LSP/Filter/Normalizer.hpp>

namespace LSP::Filter
{

// �ėʎq�� : �T���v���̃t�H�[�}�b�g��ύX
template<typename Tin, typename Tout>
struct Requantizer
{
	constexpr Tout operator()(Tin in) const noexcept 
	{
		// MEMO �ł��邾��constexpr�ŉ������A���s���R�X�g�������ɕϊ������݂̂Ƃ������B

		if constexpr (std::is_same_v<Tin, Tout>) {
			// ���͂Əo�͂̌^������ł���Εϊ��s�v
			return in;
		} else if constexpr (std::is_floating_point_v<Tin> && std::is_floating_point_v<Tout>) {
			// ���������_�����m�͒l��ϊ��\, �N���b�s���O�s�v
			return static_cast<Tout>(in);
		} else if constexpr (std::is_integral_v<Tin> && std::is_integral_v<Tout>) {
			// �������m�̓V�t�g���Z�݂̂Œl��ϊ��\, �N���b�s���O�s�v
			constexpr size_t in_bits = sizeof(Tin) * 8;
			constexpr size_t out_bits = sizeof(Tout) * 8;
			if constexpr (in_bits > out_bits) {
				// �i���[�C���O�ϊ�(�E�V�t�g)
				return static_cast<Tout>(in >> (in_bits - out_bits));
			} else {
				// ���C�h�j���O�ϊ�(���V�t�g)
				return static_cast<Tout>(in) << (out_bits - in_bits);
			}
		} else if constexpr (std::is_floating_point_v<Tin> && std::is_integral_v<Tout>) {
			// ���������_�������� : �m�[�}���C�Y���Ă��瑝��
			return static_cast<Tout>(Filter::Normalizer<Tin>()(in) * sample_traits<Tout>::abs_max);
		} else if constexpr (std::is_integral_v<Tin> && std::is_floating_point_v<Tout>) {
			// ���������������_�� : �ő�U�ꕝ�Ŋ��邾��
			return static_cast<Tout>(in) / sample_traits<Tin>::abs_max;
		} else {
			// �ϊ����Ή�
			static_assert(false, "Unsupported type.");
		}
	}
};


}