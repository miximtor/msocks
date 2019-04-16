#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <botan/stream_cipher.h>
#include <botan/auto_rng.h>

#include <msocks/utility/socket_pair.hpp>
#include <msocks/session/server_session.hpp>

#include <spdlog/spdlog.h>

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
		spdlog::info("[{}] connect to {}:{}", uuid(), host, service);
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
	error_code ec;
	utility::socket_pair(
		local_, remote_,
		ioc_,
		buffer(buffer_local_),
		[this](std::size_t n,yield_context yield)
		{
			attribute_.limiter->async_get(n, yield);
		},
		[this](mutable_buffer m_buf)
		{
			recv_cipher_->cipher1((uint8_t*)m_buf.data(), m_buf.size());
		},yield[ec]);
}

void server_session::fwd_remote_local(yield_context yield)
{
	error_code ec;
	utility::socket_pair(
		remote_, local_,
		ioc_,
		buffer(buffer_remote_),
		[this](std::size_t n,yield_context yield)
		{
			attribute_.limiter->async_get(n,yield);
		},
		[this](mutable_buffer m_buf)
		{
			send_cipher_->cipher1((uint8_t*)m_buf.data(), m_buf.size());
		}, yield[ec]);
}

void server_session::do_async_handshake(
	async_result<yield_context, void(error_code, std::pair<std::string, std::string>)>::completion_handler_type handler,
	yield_context yield)
{
	error_code ec;
	std::pair<std::string, std::string> result;
	try
	{
		Botan::AutoSeeded_RNG rng;
		std::vector<uint8_t> send_iv(8);
		rng.randomize(send_iv.data(), send_iv.size());
		async_write(local_, buffer(send_iv), transfer_all(), yield);
		cipher_setup(send_cipher_, attribute_.method, attribute_.key, send_iv);

		uint16_t size = 0;
		async_read(local_, buffer(&size, sizeof(size)), yield);
		std::vector<uint8_t> recv_iv(8);
		std::vector<uint8_t> host_service(size - 8);
		std::vector<mutable_buffer> sequence
		{
			buffer(recv_iv),
			buffer(host_service)
		};
		async_read(local_, sequence, yield);
		cipher_setup(recv_cipher_, attribute_.method, attribute_.key, recv_iv);
		recv_cipher_->decrypt(host_service);
		auto split = std::find(host_service.begin(), host_service.end(), ':');
		result.first = std::string(host_service.begin(), split);
		result.second = std::string(split + 1, host_service.end());
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