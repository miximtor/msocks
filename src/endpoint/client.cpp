#include "client.hpp"
#include <boost/asio/spawn.hpp>
#include <usings.hpp>

#include <session/client_session.hpp>

namespace msocks
{

client::client(
  const ip::tcp::endpoint &listen,
  const ip::tcp::endpoint &remote_,
  const SecByteBlock &key_) :
  acceptor(context, listen),
  remote(remote_),
  key(key_)
{
}

void client::start()
{
  
  acceptor.async_accept(
    [this](auto ec, auto socket)
    {
      if ( ec ) return;
      auto session = std::make_shared<client_session>(
        std::move(socket),
        remote,
        key);
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

