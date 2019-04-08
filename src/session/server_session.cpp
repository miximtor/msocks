#include "server_session.hpp"
#include <utility>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/udp.hpp>
#include <botan/stream_cipher.h>
#include <botan/auto_rng.h>
#include <utility/socket_pair.hpp>
#include <spdlog/spdlog.h>

namespace msocks
{

server_session::server_session(
  io_context::strand &strand_,
  ip::tcp::socket local_,
  const std::vector<uint8_t> &key_,
  std::shared_ptr<utility::limiter> limiter_) :
  session(strand_, std::move(local_), key_),
  resolver(strand.context()),
  limiter(std::move(limiter_))
{}

void server_session::start(yield_context yield)
{
  try
  {
    auto[host, service] = async_handshake(yield);
    auto result = resolver.async_resolve(host, service, yield);
    remote.async_connect(*result.begin(), yield);
    spawn(
      strand,
      [this, p = shared_from_this()](yield_context yield)
      {
        fwd_local_remote(yield);
      });
    spawn(
      strand,
      [this, p = shared_from_this()](yield_context yield)
      {
        fwd_remote_local(yield);
      });
  }
  catch ( system_error &e )
  {
    spdlog::info("[{}] error: {}",session_uuid,e.what());
  }
}

void server_session::fwd_local_remote(yield_context yield)
{
  auto ec = pair(local, remote, yield, buffer(buffer_local),
                 [this, yield](mutable_buffer b)
                 {
                   recv_cipher->cipher1((uint8_t *) b.data(), b.size());
                 });
  switch ( ec.value())
  {
    case error::eof:
    case error::operation_aborted:
    case error::connection_reset:
      break;
    default:
      break;
  }
  error_code ignored_ec;
  local.cancel(ignored_ec);
  remote.cancel(ignored_ec);
}

void server_session::fwd_remote_local(yield_context yield)
{
  auto ec = pair(remote, local, yield, buffer(buffer_remote),
                 [this, yield](mutable_buffer b)
                 {
                   limiter->async_get(b.size(), yield);
                   send_cipher->cipher1((uint8_t *) b.data(), b.size());
                 });
  switch ( ec.value())
  {
    case error::eof:
    case error::operation_aborted:
      break;
    default:
      break;
  }
  error_code ignored_ec;
  local.cancel(ignored_ec);
  remote.cancel(ignored_ec);
}

void server_session::do_async_handshake(
  server_session::Handler handler,
  yield_context yield)
{
  error_code ec;
  std::pair<std::string, std::string> result;
  try
  {
    Botan::AutoSeeded_RNG rng;
    std::vector<uint8_t> send_iv(8);
    rng.randomize(send_iv.data(), send_iv.size());
    async_write(local, buffer(send_iv), transfer_all(), yield);
    cipher_setup("ChaCha(20)", send_iv, send_cipher);
    
    uint16_t size = 0;
    async_read(local, buffer(&size, sizeof(size)), yield);
    std::vector<uint8_t> recv_iv(8);
    std::vector<uint8_t> host_service(size - 8);
    std::vector<mutable_buffer> sequence
      {
        buffer(recv_iv),
        buffer(host_service)
      };
    async_read(local, sequence , yield);
    cipher_setup("ChaCha(20)", recv_iv, recv_cipher);
    recv_cipher->decrypt(host_service);
    auto split = std::find(host_service.begin(), host_service.end(), ':');
    result.first = std::string(host_service.begin(), split);
    result.second = std::string(split + 1, host_service.end());
  }
  catch ( system_error &e )
  {
    ec = e.code();
  }
  auto cb = [handler, ec, result]()mutable
  { handler(ec, result); };
  post(strand, std::move(cb));
}

server_session::Result::return_type
server_session::async_handshake(yield_context yield)
{
  Handler handler(yield);
  Result result(handler);
  spawn(
    strand,
    [handler, this, p = shared_from_this()](yield_context yield)
    {
      do_async_handshake(handler, yield);
    });
  return result.get();
}

void server_session::go()
{
  spawn(strand, [this, p = shared_from_this()](yield_context yield)
  {
    start(yield);
  });
}

}