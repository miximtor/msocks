//
// Created by maxtorm on 2019/4/14.
//

#include <msocks/endpoint/client_endpoint.hpp>
#include <msocks/session/client_session.hpp>

namespace msocks
{

void client_endpoint::start()
{
	const ip::tcp::endpoint listen(ip::make_address_v4(cfg_.local_address), cfg_.local_port);
	static client_session_attribute attribute;
	attribute.key = cfg_.key;
	attribute.method = cfg_.method;
	attribute.timeout = cfg_.timeout;
	attribute.remote_address = cfg_.remote_address;
	attribute.remote_port = cfg_.remote_port;
	
	start_service(
		[this](ip::tcp::socket socket) -> std::shared_ptr<client_session>
		{
			return std::make_shared<client_session>(std::ref(ioc_), std::move(socket), std::ref(attribute));
		},
		listen
	);
}

}
