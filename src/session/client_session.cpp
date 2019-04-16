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
		auto host_service = utility::async_local_socks5(ioc_, local_, yield);
		spdlog::info("[{}] connect to: {}", uuid_, host_service);
		ip::tcp::endpoint ep(ip::make_address(attribute_.remote_address), attribute_.remote_port);
		remote_.async_connect(ep, yield);
		async_handshake(host_service, yield);
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
	const std::string& host_service,
	yield_context yield)
{
	async_result<yield_context, void(error_code)>::completion_handler_type handler(yield);
	async_result<yield_context, void(error_code)> result(handler);
	spawn(
		ioc_,
		[this, p = shared_from_this(), &host_service, handler](yield_context yield)
	{
		do_async_handshake(handler, host_service, yield);
	}
	);
	return result.get();
}

void client_session::fwd_remote_local(yield_context yield)
{
	error_code ec;
	utility::socket_pair(
		remote_, local_,
		ioc_,
		buffer(buffer_remote_),
		[](std::size_t, yield_context) { return; },
		[this](mutable_buffer m_buf)
		{
			recv_cipher_->cipher1((uint8_t*)m_buf.data(), m_buf.size());
		},
		yield[ec]);
}

void client_session::fwd_local_remote(yield_context yield)
{

	error_code ec;
	utility::socket_pair(
		local_, remote_,
		ioc_,
		buffer(buffer_local_),
		[](std::size_t n, yield_context) { return; },
		[this](mutable_buffer m_buf)
		{
			send_cipher_->cipher1((uint8_t*)m_buf.data(), m_buf.size());
		},
		yield[ec]);
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
	const std::string& host_service,
	yield_context yield)
{
	error_code ec;
	try
	{
		Botan::AutoSeeded_RNG rng;

		std::vector<uint8_t> send_iv(8);
		std::vector<uint8_t> recv_iv(8);

		rng.randomize(send_iv.data(), send_iv.size());

		cipher_setup(send_cipher_, attribute_.method, attribute_.key, send_iv);
		auto size = static_cast<uint16_t>(send_iv.size() + host_service.size());
		std::vector<uint8_t> temp(host_service.begin(), host_service.end());
		send_cipher_->encrypt(temp);

		std::array<const_buffer, 3> buffers
		{
			buffer(&size, sizeof(size)),
			buffer(send_iv),
			buffer(temp)
		};

		async_write(remote_, buffers, transfer_all(), yield);

		async_read(remote_, buffer(recv_iv), transfer_all(), yield);
		cipher_setup(recv_cipher_, attribute_.method, attribute_.key, recv_iv);
	}
	catch (system_error & e)
	{
		ec = e.code();
	}
	post(ioc_, std::bind(handler, ec));
}
}
