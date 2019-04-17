#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <msocks/utility/rate_limiter.hpp>
#include <msocks/session/basic_session.hpp>
#include <msocks/utility/intrusive_list_hook.hpp>

using namespace boost::asio;
using namespace boost::system;

namespace msocks
{

struct server_session_attribute
{
	server_session_attribute() : timeout(0) {};
	std::vector<uint8_t> key;
	std::string method;
	boost::posix_time::seconds timeout;
	std::size_t limit = 0;
	std::shared_ptr<utility::rate_limiter> limiter;
};

class server_session final : 
	public basic_session, 
	public std::enable_shared_from_this<server_session>,
	public utility::intrusive_list_hook<server_session>
{
public:

	server_session(io_context& ioc, ip::tcp::socket local, const server_session_attribute& attribute) :
		basic_session(ioc, std::move(local)), attribute_(attribute)
	{}

	void go();

	void notify_reuse(const io_context& ioc, ip::tcp::socket local, const server_session_attribute& attribute);

private:

	void start(yield_context yield);

	void fwd_local_remote(yield_context yield);

	void fwd_remote_local(yield_context yield);

	async_result<yield_context, void(error_code, std::pair<std::string, std::string>)>::return_type async_handshake(yield_context yield);

	void do_async_handshake(async_result<yield_context, void(error_code, std::pair<std::string, std::string>)>::completion_handler_type handler, yield_context yield);

	const server_session_attribute& attribute_;
};

}

