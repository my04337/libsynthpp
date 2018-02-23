#pragma once

#include <LSP/Base/Base.hpp>

namespace LSP
{

/// ID基本型
template<class T, typename id_type_ = uint64_t, id_type_ INITIAL_VALUE=static_cast<id_type_>(0)>
struct id_base_t final
{
	using id_type = id_type_;
	static_assert(std::is_arithmetic<id_type>::value, "invalid id_type");

	static id_base_t fromValue(id_type id) noexcept { return id_base_t(id); }

	static id_type initialValue() noexcept { return INITIAL_VALUE; }

	constexpr id_base_t() noexcept : _v(INITIAL_VALUE) {}

	constexpr bool operator==(const id_base_t& rhs)const noexcept { return _v == rhs._v; }
	constexpr bool operator!=(const id_base_t& rhs)const noexcept { return _v != rhs._v; }
	constexpr bool operator< (const id_base_t& rhs)const noexcept { return _v <  rhs._v; }

	id_base_t  operator++(int)noexcept { return id_base_t(_v++); }
	id_base_t  operator--(int)noexcept { return id_base_t(_v--); }
	id_base_t& operator++()noexcept    { ++_v; return *this; }
	id_base_t& operator--()noexcept    { --_v; return *this; }

	constexpr id_type id()const noexcept { return _v; }
	constexpr bool empty()const noexcept { return _v == INITIAL_VALUE; }
	// compare (0:equal, -1:lhs<rhs, +1:lhs>rhs)
	constexpr int compare(const id_base_t& rhs)const noexcept { return (_v == rhs._v ? 0 : (_v < rhs._v ? -1 : +1)); }

	void reset() noexcept { _v = INITIAL_VALUE;  }

private:
	explicit constexpr id_base_t(id_type v) noexcept : _v(v) {}

	id_type _v;
};

/// 発番可能ID基本型
template<class T, typename id_type_ = uint64_t, id_type_ INITIAL_VALUE=static_cast<id_type_>(0)>
struct issuable_id_base_t final
{
	using id_type = id_type_;

	static issuable_id_base_t issue() noexcept { return issuable_id_base_t(++_cur); }
	static issuable_id_base_t fromValue(id_type id) noexcept { return issuable_id_base_t(id); }
	static id_type initialValue() noexcept { return INITIAL_VALUE; }

	constexpr issuable_id_base_t() noexcept : _v(INITIAL_VALUE) {}

	constexpr bool operator==(const issuable_id_base_t& rhs)const noexcept { return _v == rhs._v; }
	constexpr bool operator!=(const issuable_id_base_t& rhs)const noexcept { return _v != rhs._v; }
	constexpr bool operator< (const issuable_id_base_t& rhs)const noexcept { return _v <  rhs._v; }

	constexpr id_type id()const noexcept { return _v; }
	constexpr bool empty()const noexcept { return _v == INITIAL_VALUE; }
	// compare (0:equal, -1:lhs<rhs, +1:lhs>rhs)
	constexpr int compare(const issuable_id_base_t& rhs)const noexcept { return (_v == rhs._v ? 0 : (_v < rhs._v ? -1 : +1)); }

	void reset() noexcept { _v = INITIAL_VALUE;  }

private:
	explicit constexpr issuable_id_base_t(id_type v) noexcept : _v(v) {}

	static std::atomic<id_type> _cur;
	id_type _v;
};
template<class T, typename id_type_, id_type_ INITIAL_VALUE>
std::atomic<id_type_> issuable_id_base_t<T, id_type_, INITIAL_VALUE>::_cur(INITIAL_VALUE);


///
}

namespace std{
template <class T, typename id_type, id_type INITIAL_VALUE>
struct hash<LSP::id_base_t<T, id_type, INITIAL_VALUE>>
{
	constexpr std::size_t operator () (const LSP::id_base_t<T,id_type, INITIAL_VALUE>& key) const noexcept { return static_cast<size_t>(key.id()); }
};
}
namespace std{
template <class T>
struct hash<LSP::issuable_id_base_t<T>>
{
	constexpr std::size_t operator () (const LSP::issuable_id_base_t<T>& key) const noexcept { return static_cast<size_t>(key.id()); }
};
}
