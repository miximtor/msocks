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
  strand(context),
  acceptor(context, listen),
  remote(remote_),
  key(key_)
{
}

void client::start()
{
  spawn(strand, [this](yield_context yield)
  {
    do_accept(yield);
  });
  context.run();
}

void client::do_accept(yield_context yield)
{
  try
  {
    while ( true )
    {
      ip::tcp::socket local(context);
      acceptor.async_accept(local, yield);
      auto session = std::make_shared<client_session>(
        strand,
        std::move(local),
        remote,
        key);
      session->go();
    }
  }
  catch ( system_error &e )
  {
    LOG(ERROR) << "on client accept: " << e.what();
  }
}

}

