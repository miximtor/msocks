//
// Created by maxtorm on 2019/4/13.
//
#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <spdlog/spdlog.h>

using namespace boost::asio;
using namespace boost::system;

namespace msocks
{

class basic_endpoint : public noncopyable
{
protected:

	explicit basic_endpoint(io_context& ioc) :
		ioc_(ioc), acceptor_(ioc)
	{}

	template <typename SessionCreate>
	void start_service(SessionCreate create, const ip::tcp::endpoint& ep)
	{
		spawn(
			ioc_,
			[create(std::move(create)), this, ep](yield_context yield)
		{
			do_async_accept(create, ep, yield);
		});
	}

	io_context& ioc_;
	ip::tcp::acceptor acceptor_;

private:

	template <typename SessionCreate>
	void do_async_accept(SessionCreate create, const ip::tcp::endpoint ep, yield_context yield)
	{
		try
		{
			acceptor_.open(ep.protocol());
			acceptor_.set_option(ip::tcp::socket::reuse_address(true));
			acceptor_.bind(ep);
			acceptor_.listen();
			while (true)
			{
				ip::tcp::socket s(ioc_);
				acceptor_.async_accept(s, yield);
				auto session = create(std::move(s));
				session->go();
			}
		}
		catch (system_error & e)
		{
			std::stringstream ss;
			ss << ep;
			spdlog::error("endpoint {}: error {}", ss.str(), e.what());
		}
	}
};
}

