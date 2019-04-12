#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <msocks/endpoint/endpoint.hpp>
namespace msocks
{

class client final : public endpoint
{
public:
  client(
    const ip::tcp::endpoint &listen,
    const ip::tcp::endpoint &remote_,
    const std::vector<uint8_t> & key_
    );
  void start();
private:
  const ip::tcp::endpoint &remote;
  const std::vector<uint8_t> &key;
};

}

