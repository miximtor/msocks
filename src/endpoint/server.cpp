#include "server.hpp"

#include <boost/asio/spawn.hpp>
#include <session/server_session.hpp>
#include <glog/logging.h>

#include <usings.hpp>

namespace msocks
{

server::server(
  const ip::tcp::endpoint &listen,
  const std::vector<uint8_t> &key_) :
  strand(context),
  acceptor(context, listen),
  key(key_)
{
}

void server::start()
{
  spawn(strand, [this](yield_context yield)
  {
    do_accept(yield);
  });
  context.run();
}

void server::do_accept(yield_context yield)
{
  while ( true )
  {
    ip::tcp::socket local(context);
    acceptor.async_accept(local, yield);
    auto session = std::make_shared<server_session>(
      strand,
      std::move(local),
      key
    );
    session->go();
  }
}
}
