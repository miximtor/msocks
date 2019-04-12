#include <boost/asio/spawn.hpp>
#include <msocks/endpoint/server.hpp>
#include <msocks/usings.hpp>

namespace msocks
{

server::server(
	const ip::tcp::endpoint &listen,
	const std::vector<uint8_t> &key_,
	std::size_t limit) :
	endpoint(listen),
	key(key_),
	limiter(new utility::limiter(strand, limit * 1024)),
	session_pool(strand)
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
				return session_pool.take(
					std::ref(strand),
					std::move(socket),
					std::cref(key),
					limiter
				);
			});
	});
	limiter->start();
	context.run();
}
}
