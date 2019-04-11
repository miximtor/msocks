#pragma once

#include <forward_list>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <spdlog/spdlog.h>
#include <usings.hpp>

namespace msocks
{

template <typename Session,typename ... Args>
void notify_reuse(Session & session,Args && ... args )
{
	session.notify_reuse(std::forward<Args>(args)...);
}

template <class Session,typename ... Args>
std::add_pointer_t<Session> raw_ptr_factory(Args && ... args)
{
	static_assert(std::is_constructible_v<Session,Args...>,"Can't construct");
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
	explicit pool(io_context &ioc) : timer(ioc)
	{
	}
	
	template <typename ... Args>
	session_shared_ptr take(Args && ... args)
	{
		session_unique_ptr session;
		if ( session_list.empty())
		{
			session_raw_ptr session_raw_ptr = raw_ptr_factory<Session>(std::forward<Args>(args)...);
			session = session_unique_ptr(session_raw_ptr, session_recycle_deleter);
			spdlog::info("pool: create session {}", session->uuid());
		}
		else
		{
			session = std::move(session_list.front());
			session_list.pop_front();
			
			notify_reuse(*session,std::forward<Args>(args)...);
			spdlog::info("pool: reuse session {}", session->uuid());
		}
		return std::move(session);
	}

private:
	
	steady_timer timer;
	
	void timed_shrink_list()
	{
	
	}
	
	void recycle(session_raw_ptr ptr)
	{
		spdlog::info("pool: recycle session: {}", ptr->uuid());
		if ( ptr == nullptr )
			return;
		session_list.emplace_front(session_unique_ptr(ptr, session_recycle_deleter));
	}
	std::forward_list<session_unique_ptr> session_list;
	
	deleter session_recycle_deleter = [this](session_raw_ptr ptr)
	{ recycle(ptr); };
};

}

