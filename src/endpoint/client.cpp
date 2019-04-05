#include "client.hpp"
#include <boost/asio/spawn.hpp>
#include <session/client_session.hpp>
#include <glog/logging.h>
#include <usings.hpp>

namespace msocks
{

client::client(
  const ip::tcp::endpoint &listen,
  const ip::tcp::endpoint &remote_,
  const std::vector<uint8_t> &key_) :
  endpoint(listen),
  remote(remote_),
  key(key_)
{
}

void client::start()
{
  spawn(strand, [this](yield_context yield)
  {
    async_accept(yield,[this](ip::tcp::socket socket) -> std::shared_ptr<client_session>
    {
      return std::make_shared<client_session>(
        strand,std::move(socket),remote,key
      );
    });
  });
  context.run();
}

}

