#pragma once

namespace msocks::utility
{
template<typename Object1>
class intrusive_list_hook
{
	template<typename Object>
	friend class intrusive_list;
	using pointer_type = std::add_pointer_t<Object1>;

	pointer_type next() const noexcept
	{
		return next_;
	}

	void next(pointer_type n) noexcept
	{
		next_ = n;
	}

	pointer_type prev() const noexcept
	{
		return prev_;
	}

	void prev(pointer_type n) noexcept
	{
		prev_ = n;
	}

	pointer_type next_ = nullptr;
	pointer_type prev_ = nullptr;
};

}
