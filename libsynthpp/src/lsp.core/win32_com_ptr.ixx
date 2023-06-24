export module lsp.core:win32_com_ptr;

export class IUnknown;

namespace lsp
{

export class basic_com_ptr
{
public:
	basic_com_ptr() {}
	basic_com_ptr(IUnknown* ptr) : _ptr(ptr) { add_ref(); }
	basic_com_ptr(const basic_com_ptr& other) : _ptr(other._ptr) { add_ref(); }
	basic_com_ptr(basic_com_ptr&& other)noexcept : _ptr(other._ptr) { other._ptr = nullptr; }
	~basic_com_ptr() { reset(); }

	basic_com_ptr& operator=(const basic_com_ptr& other) { reset(); _ptr = other._ptr; add_ref(); }
	basic_com_ptr& operator=(basic_com_ptr& other)noexcept { reset(); _ptr = other._ptr; other._ptr = nullptr; }

	void reset();

	IUnknown* get()const noexcept { return _ptr; }
	IUnknown* operator->()const noexcept { return _ptr; }
	operator IUnknown* ()const noexcept { return _ptr; }

private:
	void add_ref();

private:
	IUnknown* _ptr = nullptr;
};

export template<class T>
class com_ptr {
	com_ptr() {}
	com_ptr(T* ptr) : _ptr(ptr) {}
	com_ptr(const com_ptr&) = default;
	com_ptr(com_ptr&&)noexcept = default;
	~com_ptr() {}

	com_ptr& operator=(const com_ptr&) = default;
	com_ptr& operator=(com_ptr&&) = default;

	T* get()const noexcept { return dynamic_cast<T>(_ptr.get()); }
	T* operator->()const noexcept { return get(); }
	operator T* ()const noexcept { return get(); }

private:
	basic_com_ptr _ptr;
};
}
