#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>

using namespace boost::asio;
using namespace boost::system;

#if defined(_MSC_VER)
#define MSOCKS_PACK(MSOCKS_STRUCT_DECLARE) __pragma(pack(push, 1)) MSOCKS_STRUCT_DECLARE __pragma( pack(pop) )
#else
#define MSOCKS_PACK(MSOCKS_STRUCT_DECLARE) MSOCKS_STRUCT_DECLARE __attribute__((packed))
#endif

namespace msocks::utility
{

namespace detail
{
void do_local_socks5(
	io_context& ioc,
	ip::tcp::socket& local,
	async_result<yield_context, void(error_code, std::vector<uint8_t>)>::completion_handler_type handler,
	yield_context yield);
}

async_result<yield_context, void(error_code,std::vector<uint8_t>)>::return_type
async_local_socks5(io_context& ioc, ip::tcp::socket& local, yield_context yield);



}

