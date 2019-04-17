#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/endian/conversion.hpp>

#include <msocks/utility/socket_pair.hpp>
#include <msocks/session/server_session.hpp>
#include <msocks/utility/socks_constants.hpp>
#include <msocks/utility/socks_erorr.hpp>

#include <botan/auto_rng.h>

#include <spdlog/spdlog.h>
using namespace boost::endian;

namespace msocks
{

void server_session::start(yield_context yield)
{
	try
	{
		deadline_timer timer(ioc_);
		ip::tcp::resolver resolver(ioc_);
		spawn(
			ioc_, [this, p = shared_from_this(), &resolver, &timer](yield_context yield)
		{
			timer.expires_from_now(attribute_.timeout);
			error_code ec;
			timer.async_wait(yield[ec]);
			if (ec != error::operation_aborted)
			{
				local_.cancel(ec);
				resolver.cancel();
				remote_.cancel(ec);
			}
		});
		auto [host, service] = async_handshake(yield);
		auto result = resolver.async_resolve(host, service, yield);
		remote_.async_connect(*result.begin(), yield);
		timer.cancel();
		spawn(
			ioc_,
			[this, p = shared_from_this()](yield_context yield)
		{
			fwd_local_remote(yield);
		});
		spawn(
			ioc_,
			[this, p = shared_from_this()](yield_context yield)
		{
			fwd_remote_local(yield);
		});
	}
	catch (system_error& e)
	{
		if (e.code() != error::operation_aborted)
		{
			spdlog::info("[{}] error: {}", uuid_, e.what());
		}
	}
}

void server_session::fwd_local_remote(yield_context yield)
{
	try
	{
		error_code ec;
		utility::socket_pair(
			local_, remote_,
			ioc_,
			buffer(buffer_local_),
			[this](std::size_t n, yield_context yield)
			{
				attribute_.limiter->async_get(n, yield);
			},
			[this](mutable_buffer m_buf)
			{
				local_remote_cipher_->cipher1((uint8_t*)m_buf.data(), m_buf.size());
				printf("\n");
			}, yield[ec]);
	}
	catch (system_error & ignored)
	{

	}
}

void server_session::fwd_remote_local(yield_context yield)
{
	try
	{
		std::vector<uint8_t> iv(8);
		Botan::AutoSeeded_RNG rng;
		rng.randomize(iv.data(), iv.size());
		cipher_setup(remote_local_cipher_, attribute_.method, attribute_.key, iv);
		async_write(local_, buffer(iv), yield);

		error_code ec;
		utility::socket_pair(
			remote_, local_,
			ioc_,
			buffer(buffer_remote_),
			[this](std::size_t n, yield_context yield)
			{
				attribute_.limiter->async_get(n, yield);
			},
			[this](mutable_buffer m_buf)
			{
				remote_local_cipher_->cipher1((uint8_t*)m_buf.data(), m_buf.size());
			}, yield[ec]);
	}
	catch (system_error & ignored)
	{
	}
}

void server_session::do_async_handshake(
	async_result<yield_context, void(error_code, std::pair<std::string, std::string>)>::completion_handler_type handler,
	yield_context yield)
{
	error_code ec;
	std::pair<std::string, std::string> result;
	std::vector<uint8_t> iv(8);
	try
	{

		async_read(local_, buffer(iv), yield);
		cipher_setup(local_remote_cipher_, attribute_.method, attribute_.key, iv);
		uint8_t address_type = 0;
		async_read(local_, buffer(&address_type, sizeof(address_type)), yield);
		local_remote_cipher_->cipher1(&address_type, 1);
		if (address_type == socks::addr_ipv4)
		{
			uint32_t ipv4;
			uint16_t port;
			std::array<mutable_buffer, 2> sequence
			{
				buffer(&ipv4,sizeof(ipv4)),
				buffer(&port,sizeof(port))
			};
			async_read(local_, sequence, transfer_all(), yield);
			local_remote_cipher_->cipher1((uint8_t*)& ipv4, sizeof(ipv4));
			local_remote_cipher_->cipher1((uint8_t*)& port, sizeof(port));
			result.first = ip::make_address_v4(big_to_native(ipv4)).to_string();
			result.second = std::to_string(big_to_native(port));
		}
		else if (address_type == socks::addr_ipv6)
		{
			ip::address_v6::bytes_type ipv6;
			uint16_t port;
			std::array<mutable_buffer, 2> sequence
			{
				buffer(&ipv6,ipv6.size()),
				buffer(&port,sizeof(port))
			};
			async_read(local_, sequence, transfer_all(), yield);
			local_remote_cipher_->cipher1(ipv6.data(), ipv6.size());
			local_remote_cipher_->cipher1((uint8_t*)& port, sizeof(port));
			std::reverse(ipv6.begin(), ipv6.end());
			result.first = ip::make_address_v6(ipv6).to_string();
			result.second = std::to_string(big_to_native(port));
		}
		else if (address_type == socks::addr_domain)
		{
			uint8_t domain_length;
			std::string domain;
			uint16_t port;

			async_read(local_, buffer(&domain_length, sizeof(domain_length)), yield);
			local_remote_cipher_->cipher1(&domain_length, sizeof(domain_length));

			async_read(local_, dynamic_buffer(domain), transfer_exactly(domain_length), yield);
			local_remote_cipher_->cipher1((uint8_t*)domain.data(), domain.size());

			async_read(local_, buffer(&port, sizeof(port)), yield);
			local_remote_cipher_->cipher1((uint8_t*)& port, sizeof(port));

			result.first = domain;
			result.second = std::to_string(big_to_native(port));
		}
		else
		{
			throw system_error(errc::address_not_supported, socks_category());
		}
	}
	catch (system_error & e)
	{
		if (e.code() != error::operation_aborted)
			ec = e.code();
	}
	post(ioc_, std::bind(handler, ec, result));
}

async_result<yield_context, void(error_code, std::pair<std::string, std::string>)>::return_type
server_session::async_handshake(yield_context yield)
{
	async_result<yield_context, void(error_code, std::pair<std::string, std::string>)>::completion_handler_type handler(yield);
	async_result<yield_context, void(error_code, std::pair<std::string, std::string>)> result(handler);
	spawn(
		ioc_,
		[handler, this, p = shared_from_this()](yield_context yield)
	{
		do_async_handshake(handler, yield);
	});
	return result.get();
}


void server_session::go()
{
	spawn(
		ioc_,
		[this, p = shared_from_this()](yield_context yield)
	{
		start(yield);
	});

}
void
server_session::notify_reuse(const io_context& ioc, ip::tcp::socket local, const server_session_attribute& attribute)
{
	(void)ioc;
	(void)attribute;
	local_ = std::move(local);
	remote_ = ip::tcp::socket(ioc_);
}

}