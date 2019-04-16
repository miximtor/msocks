//
// Created by maxtorm on 2019/4/14.
//

#include <msocks/endpoint/server_endpoint.hpp>

namespace msocks
{

server_endpoint::server_endpoint(io_context &ioc, pool<server_session> &session_pool, server_endpoint_config cfg) :
	basic_endpoint(ioc),
	session_pool_(session_pool),
	cfg_(std::move(cfg)),
	limiter_(new utility::rate_limiter(ioc_,cfg_.speed_limit * 1024))
{}

void server_endpoint::start()
{
	
	const ip::tcp::endpoint listen(ip::make_address_v4(cfg_.server_address),cfg_.server_port);
	static server_session_attribute attribute;
	attribute.timeout = cfg_.timeout;
	attribute.method = cfg_.method;
	attribute.key = cfg_.key;
	attribute.limit = cfg_.speed_limit;
	attribute.limiter = limiter_;
	start_service(
		[this](ip::tcp::socket socket) -> std::shared_ptr<server_session>
		{
			socket.set_option(ip::tcp::no_delay(cfg_.no_delay));
			return session_pool_.take(std::ref(ioc_),std::move(socket),std::ref(attribute));
		},listen);
}

}

