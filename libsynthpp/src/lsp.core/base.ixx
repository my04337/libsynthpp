export module lsp.core:base;

import std;

// --- 基本的な型 ---
namespace lsp
{
// クロック類
export using clock = std::chrono::steady_clock;

/// コピー禁止,ムーブ可能型
export struct non_copy {
	constexpr non_copy() = default;
	non_copy(const non_copy&) = delete;
	non_copy(non_copy&&)noexcept = default;
	non_copy& operator=(const non_copy&) = delete;
	non_copy& operator=(non_copy&&)noexcept = default;
};

/// コピー,ムーブ禁止型
export struct non_copy_move {
	constexpr non_copy_move() = default;
	non_copy_move(const non_copy_move&) = delete;
	non_copy_move(non_copy_move&&)noexcept = delete;
	non_copy_move& operator=(const non_copy_move&) = delete;
	non_copy_move& operator=(non_copy_move&&)noexcept = delete;
};

// テンプレート補助 : どのような値を受け取ってもfalseを表す値
export template<typename ...>
constexpr bool false_v = false;

/// スコープ離脱時実行コード 補助クラス
export template<typename F>
class [[nodiscard]] _finally_action
	: non_copy
{
public:
	_finally_action() : f(), valid(false) {}
	_finally_action(F f) : f(std::move(f)), valid(true) {}
	~_finally_action() { action(); }

	_finally_action(_finally_action&& d)noexcept : f(std::move(d.f)), valid(true) { d.valid = false; };
	_finally_action& operator=(_finally_action&& d)noexcept { reset(); f = std::move(d.f); valid = d.valid; d.valid = false; return *this; }

	void action()noexcept { if(valid) f(); reset(); }
	void reset()noexcept { valid = false; }

private:
	F f;
	bool valid;
};

/// スコープ離脱時実行コード 定義関数
export template<typename F>
_finally_action<F> finally(F&& f) { return { std::forward<F>(f) }; }

/// 型名をデマングルした文字列に変換します
export std::string demangle(const char* mangled_name);
export inline std::string demangle(const std::type_info& v) { return lsp::demangle(v.name()); }
export inline std::string demangle(const std::type_index& v) { return lsp::demangle(v.name()); }


// std::pmr_memory_holder メモリ管理簡単化機構
export template<class T>
class _memory_resource_deleter final
{
public:
	_memory_resource_deleter()
		: _mem(nullptr), _size(0), _align(0)
	{}
	_memory_resource_deleter(std::pmr::memory_resource* mem, size_t size, size_t align)
		: _mem(mem), _size(size), _align(align)
	{}

	void operator()(T* data) {
		if(data) _mem->deallocate(data, _size, _align);
	}

private:
	std::pmr::memory_resource* _mem;
	size_t _size;
	size_t _align;
};

export template<class T>
std::unique_ptr<T[], _memory_resource_deleter<T>>
	allocate_memory(std::pmr::memory_resource* mem, size_t length, size_t align = alignof(T))
{
	size_t size = length * sizeof(T);
	auto data = reinterpret_cast<T*>(mem->allocate(size, align));
	return std::unique_ptr<T[], _memory_resource_deleter<T>>(data, _memory_resource_deleter<T>(mem, size, align));
}

// 添字演算子可能である事を表すコンセプト
export template<typename Tcontainer, typename Telement>
concept subscript_operator_available = requires(Tcontainer a, size_t index) {
	{a[index]} -> std::convertible_to<Telement>;
};
}
