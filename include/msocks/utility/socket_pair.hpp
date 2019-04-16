#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/streambuf.hpp>

using namespace boost::asio;
using namespace boost::system;

namespace msocks::utility
{
template <typename CompletionToken>
async_result<yield_context, void(error_code)>::return_type
socket_pair(
	ip::tcp::socket& source,
	ip::tcp::socket& destination,
	io_context& ioc,
	mutable_buffer m_buf,
	std::function<void(std::size_t, yield_context)> before_read,
	std::function<void(mutable_buffer)> before_write,
	CompletionToken&& token
)
{
	using result_type = async_result<CompletionToken, void(error_code)>;
	using handler_type = typename result_type::completion_handler_type;
	handler_type handler(std::forward<CompletionToken>(token));
	result_type result(handler);
	spawn(
		ioc,
		[
			&source,
			&destination,
			&ioc,
			m_buf,
			before_read(std::move(before_read)),
			before_write(std::move(before_write)),
			handler
		](yield_context yield) mutable
	{
		error_code ec;
		while (true)
		{
			auto n_read = source.async_read_some(m_buf, yield[ec]);
			if (ec)
			{
				break;
			}
			before_read(n_read, yield);
			before_write(buffer(m_buf, n_read));
			async_write(destination, buffer(m_buf, n_read), yield[ec]);
			if (ec)
			{
				break;
			}
		}
		post(ioc, std::bind(handler, ec));
	});
	return result.get();
}

}
