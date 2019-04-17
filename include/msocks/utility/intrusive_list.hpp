#pragma once
#include <boost/noncopyable.hpp>
#include <msocks/utility/intrusive_list_hook.hpp>
namespace msocks::utility
{

template<typename Object>
class intrusive_list;

template<typename Object>
class intrusive_list : public boost::noncopyable
{
	static_assert(std::is_base_of_v<intrusive_list_hook<Object>, Object>, "Object must be an intrusive list");
public:

	using pointer_type = typename intrusive_list_hook<Object>::pointer_type;

	std::size_t size() const noexcept
	{
		return size_;
	}

	bool empty() const noexcept
	{
		return  size() == 0;
	}

	pointer_type take() noexcept
	{
		if (begin_ == nullptr)
			return nullptr;
		pointer_type object = begin_;
		--size_;
		if (begin_ == end_)
		{
			begin_ = end_ = nullptr;
		}
		else
		{
			begin_ = begin_->next();
		}
		return object;
	}

	pointer_type release() noexcept
	{
		if (end_ == nullptr)
			return nullptr;
		pointer_type object = end_;
		--size_;
		if (begin_ == end_)
		{
			begin_ = end_ = nullptr;
		}
		else
		{
			end_ = end_->prev();
		}
		return object;
	}

	void offer(pointer_type object) noexcept
	{
		if (object == nullptr)
			return;

		++size_;
		if (begin_ == nullptr)
		{
			end_ = begin_ = object;
			return;
		}
		object->prev(nullptr);
		object->next(begin_);
		begin_->prev(object);
		begin_ = begin_->prev();
	}

private:
	pointer_type begin_ = nullptr;
	pointer_type end_ = nullptr;
	std::size_t size_ = 0;
};

}