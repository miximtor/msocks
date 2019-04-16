#pragma once
#include <boost/noncopyable.hpp>
namespace msocks::utility
{

template<typename Object>
class intrusive_list;

template<typename Object1>
class intrusive_list_node
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

template<typename Object>
class intrusive_list : public boost::noncopyable
{
	static_assert(std::is_base_of_v<intrusive_list_node<Object>,Object>,"Object must be an intrusive list");
public:

	using pointer_type = typename intrusive_list_node<Object>::pointer_type;

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
			begin_=end_=nullptr;
		}
		else
		{
			begin_ = begin_->next();
		}
		return object;
	}

	void offer(pointer_type object) noexcept
	{
		if (object == nullptr)
			return;

		++size_;
		if (end_==nullptr)
		{
			end_ = begin_ = object;
			return;
		}
		end_->next(object);
		object->prev(end_);
		object->next(nullptr);
		end_ = end_->next();
	}

private:
	pointer_type begin_ = nullptr;
	pointer_type end_ = nullptr;
	std::size_t size_ = 0;
};

}