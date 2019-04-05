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
  endpoint(listen),
  key(key_)
{
}

void server::start()
{
  spawn(strand, [this](yield_context yield)
  {
    async_accept(yield, [this](ip::tcp::socket socket)  -> std::shared_ptr<server_session>
    {
      return std::make_shared<server_session>(
        strand,
        std::move(socket),
        key
      );
    });
  });
  context.run();
}
}
