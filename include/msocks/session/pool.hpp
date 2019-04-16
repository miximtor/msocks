#pragma once

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/spawn.hpp>

#include <msocks/utility/intrusive_list.hpp>

#include <spdlog/spdlog.h>

using namespace boost::asio;
using namespace boost::system;

namespace msocks
{

template <typename Session, typename ... Args>
void notify_reuse(Session& session, Args&& ... args)
{
	session.notify_reuse(std::forward<Args>(args)...);
}

template <class Session, typename ... Args>
std::add_pointer_t<Session> raw_ptr_factory(Args&& ... args)
{
	static_assert(std::is_constructible_v<Session, Args...>, "Can't construct");
	return new Session(std::forward<Args>(args)...);
}

template <typename Session>
class pool
{
	using pointer_type = typename utility::intrusive_list<Session>::pointer_type;
	using deleter_type = std::function<void(pointer_type)>;
	using unique_pointer_type = std::unique_ptr<Session, deleter_type>;
	using shared_pointer_type = std::shared_ptr<Session>;
public:

	explicit pool(io_context& ioc) : ioc_(ioc), timer_(ioc)
	{
		timed_shrink_list();
	}

	template <typename ... Args>
	shared_pointer_type take(Args&& ... args);

private:
	io_context& ioc_;
	steady_timer timer_;
	std::size_t out_ = 0;

	void timed_shrink_list();

	void recycle(pointer_type session);

	utility::intrusive_list<Session> session_list_;

};


template <typename Session>
template <typename ... Args>
typename pool<Session>::shared_pointer_type pool<Session>::take(Args&& ... args)
{
	pointer_type session;
	if (session_list_.empty())
	{
		session = raw_ptr_factory<Session>(std::forward<Args>(args)...);
	}
	else
	{
		session = session_list_.take();
		notify_reuse(*session, std::forward<Args>(args)...);
	}
	out_++;
	return unique_pointer_type(session,[this](pointer_type session){recycle(session);});
}

template <typename Session>
void pool<Session>::recycle(pointer_type session)
{
	if (session == nullptr)
		return;
	out_--;
	session_list_.offer(session);
}

template <typename Session>
void pool<Session>::timed_shrink_list()
{
	spawn(
		ioc_,
		[this](yield_context yield)
		{
			while (true)
			{
				timer_.expires_after(std::chrono::seconds(30));
				error_code ec;
				timer_.async_wait(yield[ec]);
				spdlog::info("pool: ready release {} session", session_list_.size() > out_ / 2 ? session_list_.size() - out_ / 2 : 0);
				while (session_list_.size() > out_ / 2)
				{
					pointer_type session = session_list_.take();
					delete session;
				}
			}
		});
}

}
