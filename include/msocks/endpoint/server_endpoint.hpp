//
// Created by maxtorm on 2019/4/13.
//
#pragma once

#include <msocks/endpoint/basic_endpoint.hpp>
#include <msocks/session/pool.hpp>
#include <msocks/session/server_session.hpp>
#include <msocks/utility/rate_limiter.hpp>

namespace msocks
{

struct server_endpoint_config
{
	server_endpoint_config() : timeout(0) {};
	std::string server_address;
	uint16_t server_port = 0;
	size_t speed_limit = 0;
	std::vector<uint8_t> key;
	bool no_delay = true;
	std::string method;
	boost::posix_time::seconds timeout;
};

class server_endpoint final : public basic_endpoint
{
public:
	server_endpoint(io_context& ioc, pool<server_session>& session_pool, server_endpoint_config cfg);

	void start();
private:
	pool<server_session>& session_pool_;
	server_endpoint_config cfg_;
	std::shared_ptr<utility::rate_limiter> limiter_;
};

}
