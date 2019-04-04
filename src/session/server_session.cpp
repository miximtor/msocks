#include "server_session.hpp"

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/udp.hpp>

#include <glog/logging.h>

namespace msocks
{

server_session::server_session(
  ip::tcp::socket local_,
  const CryptoPP::SecByteBlock& key_) :
  local(std::move(local_)),
  remote(local.get_executor().context()),
  key(key_),
  iv(8)
{}

void server_session::start(yield_context &yield)
{
  try
  {
    auto[host, service] = do_get_target(yield);
    ip::tcp::resolver resolver(local.get_executor().context());
    auto result = resolver.async_resolve(host, service, yield);
    remote.async_connect(*result.begin(), yield);
  }
  catch ( system_error &e )
  {
    local.close();
    remote.close();
    LOG(WARNING) << e.what();
    return;
  }
  auto p = shared_from_this();
  spawn(
    yield,
    [this, p](yield_context yield)
    {
      do_forward_local_to_remote(yield);
    });
  
  spawn(
    yield,
    [this, p](yield_context yield)
    {
      do_forward_remote_to_local(yield);
    });
  
}

void server_session::do_forward_local_to_remote(yield_context &yield)
{
  try
  {
    std::vector<uint8_t> buf;
    while ( true )
    {
      auto n_read = async_read(local, dynamic_buffer(buf), yield);
      decrypt.ProcessString(buf.data(), n_read);
      async_write(remote, buffer(buf), transfer_all(), yield);
      buf.clear();
    }
  }
  catch ( system_error &e )
  {
    local.close();
    remote.close();
    LOG(WARNING) << e.what();
  }
}

void server_session::do_forward_remote_to_local(yield_context &yield)
{
  try
  {
    std::vector<uint8_t> buf;
    while ( true )
    {
      auto n_read = async_read(remote, dynamic_buffer(buf), yield);
      encrypt.ProcessString(buf.data(), n_read);
      async_write(local, buffer(buf), transfer_all(), yield);
      buf.clear();
    }
  }
  catch ( system_error &e )
  {
    local.close();
    remote.close();
    LOG(WARNING) << e.what();
  }
}

std::pair<std::string, std::string>
server_session::do_get_target(yield_context &yield)
{
  uint16_t size = 0;
  async_read(local, buffer(&size, sizeof(size)), yield);
  
  std::string addr_port(size - 8, '\0');
  
  std::vector<mutable_buffer> sequence;
  sequence.emplace_back(buffer(iv.data(), iv.size()));
  sequence.emplace_back(buffer(addr_port));
  
  async_read(local, sequence, transfer_all(), yield);
  
  encrypt.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());
  decrypt.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());
  
  decrypt.ProcessString((byte *) addr_port.data(), addr_port.size());
  
  auto split = addr_port.find(':');
  return {
    addr_port.substr(0, split),
    addr_port.substr(split + 1)
  };
}

}