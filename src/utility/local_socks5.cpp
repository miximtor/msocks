//
// Created by maxtorm on 2019/4/14.
//

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/endian/conversion.hpp>

#include <msocks/utility/socks_constants.hpp>
#include <msocks/utility/socks_erorr.hpp>
#include <msocks/utility/local_socks5.hpp>
#include <spdlog/spdlog.h>
using namespace boost::endian;

namespace msocks::utility
{
void detail::do_local_socks5(
	io_context& strand,
	ip::tcp::socket& local,
	async_result<yield_context, void(error_code, std::string)>::completion_handler_type handler,
	yield_context yield)
{
	std::array<uint8_t, 256 + 2> temp{};
	MSOCKS_PACK(
		struct
	{
		uint8_t version;
		uint8_t n_method;
	})
		auth_method {};
	MSOCKS_PACK(
		struct
	{
		uint8_t version;
		uint8_t method;
	}
	) auth_reply {
		socks::socks5_version, socks::auth_no_auth
	};

	MSOCKS_PACK(
		struct
	{
		uint8_t version;
		uint8_t cmd;
		uint8_t reserve;
		uint8_t addr_type;
	})
		request {};

	MSOCKS_PACK(
		struct
	{
		uint8_t version;
		uint8_t rep;
		uint8_t rsv;
		uint8_t atyp;
		uint32_t bnd_addr;
		uint16_t bnd_port;
	})
		reply {
		socks::socks5_version, 0x00, 0x00, socks::addr_ipv4, 0x00000000, 0x0000
	};

	std::string result;
	error_code ec;
	try
	{
		async_read(local, buffer(&auth_method, sizeof(auth_method)), yield);
		async_read(local, buffer(temp, auth_method.n_method), yield);
		async_write(local, buffer(&auth_reply, sizeof(auth_reply)), yield);
		async_read(local, buffer(&request, sizeof(request)), yield);
		switch (request.cmd)
		{
			case socks::conn_tcp:
				break;
			case socks::conn_bind:
			case socks::conn_udp:
			default:
				throw system_error(errc::cmd_not_supported, socks_category());
		}

		if (request.addr_type == socks::addr_ipv4)
		{
			async_read(local, buffer(temp, 32 / 8 + 2), yield);
			MSOCKS_PACK(
				struct
			{
				uint32_t addr;
				uint16_t port;
			}
			)
				address_port {};

			buffer_copy(buffer(&address_port, sizeof(address_port)), buffer(temp),
									sizeof(address_port));
			result =
				ip::make_address_v4(big_to_native(address_port.addr)).to_string() +
				":" +
				std::to_string(big_to_native(address_port.port));
		}
		else if (request.addr_type == socks::addr_ipv6)
		{
			async_read(local, buffer(temp, 128 / 8 + 2), yield);
			MSOCKS_PACK(
				struct
			{
				ip::address_v6::bytes_type addr;
				uint16_t port;
			})
				address_port {};
			buffer_copy(buffer(&address_port, sizeof(address_port)), buffer(temp),
									sizeof(address_port));
			std::reverse(address_port.addr.begin(), address_port.addr.end());
			result =
				ip::make_address_v6(address_port.addr).to_string() +
				":" +
				std::to_string(big_to_native(address_port.port));
		}
		else if (request.addr_type == socks::addr_domain)
		{
			uint8_t domain_length;
			async_read(local, buffer(&domain_length, sizeof(domain_length)), yield);
			async_read(local, dynamic_buffer(result), transfer_exactly(domain_length),
								 yield);
			uint16_t port = 0;
			async_read(local, buffer(&port, sizeof(port)), yield);
			big_to_native_inplace(port);
			result += ":" + std::to_string(port);
		}
		else
		{
			throw system_error(errc::address_not_supported, socks_category());
		}
		async_write(local, buffer(&reply, sizeof(reply)), yield);
	}
	catch (system_error& e)
	{
		ec = e.code();
	}
	post(strand, std::bind(handler, ec, result));
}

async_result<yield_context, void(error_code, std::string)>::return_type
async_local_socks5(io_context &ioc, ip::tcp::socket &local, yield_context yield)
{
	async_result<yield_context, void(error_code,std::string)>::completion_handler_type handler(yield);
	async_result<yield_context, void(error_code,std::string)> result(handler);
	spawn(ioc, std::bind(&detail::do_local_socks5,std::ref(ioc),std::ref(local),handler,std::placeholders::_1));
	return result.get();
}
}
