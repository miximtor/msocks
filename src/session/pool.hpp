#pragma once

#include <forward_list>
#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <spdlog/spdlog.h>
#include <usings.hpp>

namespace msocks
{

template <typename Session>
class pool
{
private:
	using session_raw_ptr = std::add_pointer_t<Session>;
	using deleter = std::function<void(session_raw_ptr)>;
	using session_unique_ptr = std::unique_ptr<Session, deleter>;
	using session_shared_ptr = std::shared_ptr<Session>;
	using SessionRawPtrFactory = std::function<session_raw_ptr(ip::tcp::socket)>;
public:
	pool(io_context &ioc, SessionRawPtrFactory raw_ptr_factory_) :
		timer(ioc), raw_ptr_factory(std::move(raw_ptr_factory_))
	{
	}
	
	session_shared_ptr take(ip::tcp::socket socket)
	{
		session_unique_ptr session;
		if ( session_list.empty())
		{
			session_raw_ptr ptr = raw_ptr_factory(std::move(socket));
			session = session_unique_ptr(ptr, session_recycle_deleter);
			spdlog::info("pool: create a new session {}", session->uuid());
		}
		else
		{
			session = std::move(session_list.front());
			session->reuse(std::move(socket));
			session_list.pop_front();
			spdlog::info("pool: reuse a old session {}", session->uuid());
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
		spdlog::info("recycle session: {}", ptr->uuid());
		if ( ptr == nullptr )
			return;
		session_list.emplace_front(session_unique_ptr(ptr, session_recycle_deleter));
	}
	
	std::forward_list<session_unique_ptr> session_list;
	
	SessionRawPtrFactory raw_ptr_factory;
	deleter session_recycle_deleter = [this](session_raw_ptr ptr)
	{ recycle(ptr); };
};

}

