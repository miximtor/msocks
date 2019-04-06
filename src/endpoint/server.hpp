#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <utility/limiter.hpp>
#include <usings.hpp>
#include "endpoint.hpp"
namespace msocks
{
class server : public endpoint
{
public:
  server(const ip::tcp::endpoint &listen,const std::vector<uint8_t> & key_);
  void start();
private:
  const std::vector<uint8_t> &key;
  std::shared_ptr<utility::limiter> limiter;
};
}


