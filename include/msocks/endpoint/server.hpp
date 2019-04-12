#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <msocks/session/pool.hpp>
#include <msocks/utility/limiter.hpp>
#include <msocks/session/server_session.hpp>
#include <msocks/endpoint/endpoint.hpp>
#include <msocks/usings.hpp>
namespace msocks
{
class server : public endpoint
{
public:
  server(const ip::tcp::endpoint &listen,const std::vector<uint8_t> & key_,std::size_t limit);
  void start();
private:
  const std::vector<uint8_t> &key;
  std::shared_ptr<utility::limiter> limiter;
	pool<server_session> session_pool;
};
}


