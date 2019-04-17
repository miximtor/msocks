#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <boost/endian/conversion.hpp>
#include <botan/auto_rng.h>

#include <msocks/session/client_session.hpp>
#include <msocks/utility/socket_pair.hpp>
#include <msocks/utility/local_socks5.hpp>

#include <spdlog/spdlog.h>

namespace msocks
{

void client_session::start(yield_context yield)
{
	try
	{
		auto target_address = utility::async_local_socks5(ioc_, local_, yield);
		ip::tcp::endpoint ep(ip::make_address(attribute_.remote_address), attribute_.remote_port);
		remote_.async_connect(ep, yield);
		async_handshake(target_address, yield);
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
		spdlog::info("[{}] error: {}", uuid(), e.what());
	}
}

async_result<yield_context, void(error_code)>::return_type client_session::async_handshake(
	const std::vector<uint8_t>& target_address,
	yield_context yield)
{
	async_result<yield_context, void(error_code)>::completion_handler_type handler(yield);
	async_result<yield_context, void(error_code)> result(handler);
	spawn(
		ioc_,
		[this, p = shared_from_this(), target_address, handler](yield_context yield)
	{
		do_async_handshake(handler, target_address, yield);
	}
	);
	return result.get();
}

void client_session::fwd_remote_local(yield_context yield)
{
	try
	{
		std::vector<uint8_t> iv(8);
		async_read(remote_, buffer(iv), yield);
		cipher_setup(remote_local_cipher_, attribute_.method, attribute_.key, iv);
		
		error_code ec;
		utility::socket_pair(
			remote_, local_,
			ioc_,
			buffer(buffer_remote_),
			[](std::size_t, yield_context) { return; },
			[this](mutable_buffer m_buf)
			{
				remote_local_cipher_->cipher1((uint8_t*)m_buf.data(), m_buf.size());
			},
			yield[ec]);

	}
	catch (system_error & ignored)
	{
	}
}

void client_session::fwd_local_remote(yield_context yield)
{
	try
	{
		error_code ec;
		utility::socket_pair(
			local_, remote_,
			ioc_,
			buffer(buffer_local_),
			[](std::size_t n, yield_context) { return; },
			[this](mutable_buffer m_buf)
			{
				local_remote_cipher_->cipher1((uint8_t*)m_buf.data(), m_buf.size());
			},
			yield[ec]);

	}
	catch (system_error & ignored)
	{
	}
}

void client_session::go()
{
	spawn(
		ioc_,
		[this, p = shared_from_this()](yield_context yield)
	{
		start(yield);
	}
	);
}

void client_session::do_async_handshake(
	async_result<yield_context, void(error_code)>::completion_handler_type handler,
	const std::vector<uint8_t>& target_address,
	yield_context yield)
{
	error_code ec;
	try
	{
		Botan::AutoSeeded_RNG rng;
		std::vector<uint8_t> iv(8);
		rng.randomize(iv.data(), iv.size());
		cipher_setup(local_remote_cipher_, attribute_.method, attribute_.key, iv);
		auto target_address_encrypted = target_address;
		local_remote_cipher_->encrypt(target_address_encrypted);
		std::array<const_buffer, 2> buffers
		{
			buffer(iv),
			buffer(target_address_encrypted)
		};
		async_write(remote_, buffers, transfer_all(), yield);
	}
	catch (system_error & e)
	{
		ec = e.code();
	}
	post(ioc_, std::bind(handler, ec));
}
}
