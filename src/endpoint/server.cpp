#include "server.hpp"

#include <boost/asio/spawn.hpp>
#include <usings.hpp>

namespace msocks
{

server::server(
	const ip::tcp::endpoint &listen,
	const std::vector<uint8_t> &key_,
	std::size_t limit) :
	endpoint(listen),
	key(key_),
	limiter(new utility::limiter(strand, limit * 1024)),
	session_pool(
		context,
		[this](ip::tcp::socket s) -> server_session *
		{
			return new server_session(strand,std::move(s),key,limiter);
		})
{
}

void server::start()
{
	spawn(strand, [this](yield_context yield)
	{
		async_accept(
			yield,
			[this](ip::tcp::socket socket)
			{
				return session_pool.take(std::move(socket));
			});
	});
	limiter->start();
	context.run();
}
}
