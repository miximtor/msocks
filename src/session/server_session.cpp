#include "server_session.hpp"

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/udp.hpp>
#include <botan-2/botan/stream_cipher.h>
#include <utility/dup.hpp>
#include <glog/logging.h>
#include <botan/auto_rng.h>

namespace msocks
{

server_session::server_session(
  io_context::strand &strand_,
  ip::tcp::socket local_,
  const std::vector<uint8_t> &key_) :
  strand(strand_),
  local(std::move(local_)),
  remote(local.get_executor().context()),
  key(key_),
  resolver(strand.context())
{}

void server_session::start(yield_context yield)
{
  try
  {
    auto[host, service] = handshake(yield);
    LOG(INFO) << "socket connect to: " << host << ":" << service;
    auto result = resolver.async_resolve(host, service, yield);
    remote.async_connect(*result.begin(), yield);
    spawn(
      strand,
      [this, p = shared_from_this()](yield_context yield)
      {
        do_forward_local_to_remote(yield);
      });
    spawn(
      strand,
      [this, p = shared_from_this()](yield_context yield)
      {
        do_forward_remote_to_local(yield);
      });
  }
  catch ( system_error &e )
  {
    LOG(INFO) << e.what();
  }
}

void server_session::do_forward_local_to_remote(yield_context yield)
{
  std::vector<uint8_t> buf(2048);
  auto ec = pair(local, remote, yield, buf, [this](mutable_buffer b)
  {
    recv_cipher->cipher1((uint8_t *) b.data(), b.size());
  });
  LOG(INFO) << ec.message();
  local.close();
  remote.close();
}

void server_session::do_forward_remote_to_local(yield_context yield)
{
  std::vector<uint8_t> buf(2048);
  auto ec = pair(remote, local, yield, buf, [this](mutable_buffer b)
  {
    send_cipher->cipher1((uint8_t *) b.data(), b.size());
  });
  LOG(INFO) << ec.message();
  local.close();
  remote.close();
}

std::pair<std::string, std::string>
server_session::handshake(yield_context yield)
{
  uint16_t size = 0;
  async_read(local, buffer(&size, sizeof(size)), yield);
  
  std::vector<uint8_t> recv_iv(8);
  std::vector<uint8_t> host_service(size - 8);
  
  std::vector<mutable_buffer> sequence;
  sequence.emplace_back(buffer(recv_iv.data(), recv_iv.size()));
  sequence.emplace_back(buffer(host_service));
  async_read(local, sequence, transfer_all(), yield);
  
  recv_cipher = Botan::StreamCipher::create_or_throw("ChaCha(20)");
  recv_cipher->set_key(key.data(), key.size());
  recv_cipher->set_iv(recv_iv.data(), recv_iv.size());
  recv_cipher->decrypt(host_service);
  
  Botan::AutoSeeded_RNG rng;
  std::vector<uint8_t> send_iv(8);
  
  rng.randomize(send_iv.data(), send_iv.size());
  async_write(local, buffer(send_iv), transfer_all(), yield);
  
  send_cipher = Botan::StreamCipher::create_or_throw("ChaCha(20)");
  send_cipher->set_key(key.data(), key.size());
  send_cipher->set_iv(send_iv.data(), send_iv.size());
  
  auto split = std::find(host_service.begin(), host_service.end(), ':');
  return {
    std::string(host_service.begin(), split),
    std::string(split + 1, host_service.end())
  };
}

void server_session::go()
{
  spawn(strand, [this, p = shared_from_this()](yield_context yield)
  {
    start(yield);
  });
}

}