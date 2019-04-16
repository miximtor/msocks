#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <msocks/session/basic_session.hpp>

using namespace boost::system;
using namespace boost::asio;

namespace msocks
{

struct client_session_attribute
{
	client_session_attribute() : timeout(0) {};
	std::string remote_address;
	uint16_t remote_port = 0;
	std::vector<uint8_t> key;
	std::string method;
	boost::posix_time::seconds timeout;
};

class client_session final : public basic_session, public std::enable_shared_from_this<client_session>
{
public:
	client_session(io_context& ioc, ip::tcp::socket local, const client_session_attribute& attribute) :
		basic_session(ioc, std::move(local)), attribute_(attribute)
	{}

	void go();
private:

	void start(yield_context yield);

	async_result<yield_context, void(error_code)>::return_type async_handshake(const std::string& host_service, yield_context yield);

	void do_async_handshake(async_result<yield_context, void(error_code)>::completion_handler_type handler, const std::string& host_service, yield_context yield);

	void fwd_local_remote(yield_context yield);
	void fwd_remote_local(yield_context yield);

	const client_session_attribute& attribute_;

};
}
