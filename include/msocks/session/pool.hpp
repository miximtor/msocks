#pragma once

#include <deque>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/strand.hpp>
#include <spdlog/spdlog.h>
#include <msocks/usings.hpp>

namespace msocks
{

template <typename Session, typename ... Args>
void notify_reuse(Session &session, Args &&... args)
{
	session.notify_reuse(std::forward<Args>(args)...);
}

template <class Session, typename ... Args>
std::add_pointer_t<Session> raw_ptr_factory(Args &&... args)
{
	static_assert(std::is_constructible_v<Session, Args...>, "Can't construct");
	return new Session(std::forward<Args>(args)...);
}

template <typename Session>
class pool
{
private:
	using session_raw_ptr = std::add_pointer_t<Session>;
	using deleter = std::function<void(session_raw_ptr)>;
	using session_unique_ptr = std::unique_ptr<Session, deleter>;
	using session_shared_ptr = std::shared_ptr<Session>;
public:
	explicit pool(io_context::strand &strand_) : strand(strand_),timer(strand.context())
	{
		timed_shrink_list();
	}
	
	template <typename ... Args>
	session_shared_ptr take(Args &&... args)
	{
		session_raw_ptr session;
		if ( session_list.empty())
		{
			session = raw_ptr_factory<Session>(std::forward<Args>(args)...);
			spdlog::info("pool: create session {}", session->uuid());
		}
		else
		{
			session = session_list.front();
			session_list.pop_front();
			notify_reuse(*session, std::forward<Args>(args)...);
			spdlog::info("pool: reuse session {}", session->uuid());
		}
		out++;
		return session_unique_ptr(session, session_recycle_deleter);
	}

private:
	io_context::strand & strand;
	steady_timer timer;
	std::size_t out = 0;
	
	void timed_shrink_list()
	{
		spawn(strand,[this](yield_context yield)
		{
			while(true)
			{
				timer.expires_after(std::chrono::seconds(30));
				error_code ec;
				timer.async_wait(yield[ec]);
				spdlog::info("pool: ready release {} session",session_list.size() > out/2?session_list.size()-out/2:0);
				while (session_list.size() > out/2)
				{
					auto p = session_list.back();
					session_list.pop_back();
					spdlog::info("pool: release session {}",p->uuid());
					delete p;
				}
			}
		});
	}
	
	void recycle(session_raw_ptr ptr)
	{
		if ( ptr == nullptr )
			return;
		out--;
		spdlog::info("pool: recycle session: {}", ptr->uuid());
		session_list.push_front(ptr);
	}
	
	std::deque<session_raw_ptr> session_list;
	deleter session_recycle_deleter = [this](session_raw_ptr ptr)
	{ recycle(ptr); };
};

}

