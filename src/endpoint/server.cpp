#include "server.hpp"

#include <boost/asio/spawn.hpp>
#include <session/server_session.hpp>
#include <usings.hpp>

namespace msocks
{

server::server(const ip::tcp::endpoint &listen, const SecByteBlock &key_) :
  acceptor(context, listen),
  key(key_)
{
}

void server::start()
{
  acceptor.async_accept(
    [this](auto ec, auto socket)
    {
      if ( ec ) return;
      auto session = std::make_shared<server_session>(std::move(socket), key);
      spawn(
        context,
        [session](yield_context yield)
        {
          session->start(yield);
        });
    });
  context.run();
}

}
