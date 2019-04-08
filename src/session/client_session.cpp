#include "client_session.hpp"
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/endian/conversion.hpp>
#include <botan/auto_rng.h>
#include <utility/socks_constants.hpp>
#include <utility/socket_pair.hpp>
#include <utility/local_socks5.hpp>

namespace msocks
{

void client_session::start(yield_context yield)
{
  try
  {
    auto host_service = utility::async_local_socks5(strand, local, yield);
    spdlog::info("[{}] connect to: {}",session_uuid,host_service);
    remote.async_connect(addr, yield);
    async_handshake(host_service, yield);
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

client_session::client_session(
  io_context::strand &strand_,
  ip::tcp::socket local_,
  const ip::tcp::endpoint &addr_,
  const std::vector<uint8_t> &key_) :
  session(strand_, std::move(local_), key_),
  addr(addr_)
{
}


void client_session::async_handshake(
  const std::string &host_service,
  yield_context yield)
{
  Handler handler(yield);
  Result result(handler);
  spawn(
    strand,
    [this, p = shared_from_this(), &host_service, handler](yield_context yield)
    {
      do_async_handshake(handler, host_service, yield);
    }
  );
  return result.get();
}

void client_session::fwd_remote_local(yield_context yield)
{
  auto ec = pair(remote, local, yield, buffer(buffer_remote),
                 [this](mutable_buffer b)
                 {
                   recv_cipher->cipher1((uint8_t *) b.data(), b.size());
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

void client_session::fwd_local_remote(yield_context yield)
{
  auto ec = pair(
    local, remote,
    yield,
    buffer(buffer_local),
    [this](mutable_buffer b)
    {
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

void client_session::go()
{
  spawn(strand, [this, p = shared_from_this()](yield_context yield)
  {
    start(yield);
  });
}

void client_session::do_async_handshake(
  client_session::Handler handler,
  const std::string &host_service,
  yield_context yield)
{
  error_code ec;
  try
  {
    Botan::AutoSeeded_RNG rng;
    std::vector<uint8_t> send_iv(8);
    std::vector<uint8_t> recv_iv(8);
    rng.randomize(send_iv.data(), send_iv.size());
    cipher_setup("ChaCha(20)", send_iv, send_cipher);
    uint16_t size = send_iv.size() + host_service.size();
    std::vector<uint8_t> temp(host_service.begin(), host_service.end());
    send_cipher->encrypt(temp);
    std::vector<const_buffer> buffers
      {
        buffer(&size, sizeof(size)),
        buffer(send_iv),
        buffer(temp)
      };
    async_write(remote, buffers, transfer_all(), yield);
    async_read(remote, buffer(recv_iv), transfer_all(), yield);
    cipher_setup("ChaCha(20)", recv_iv, recv_cipher);
  }
  catch ( system_error &e )
  {
    ec = e.code();
  }
  auto cb = [ec,handler]()mutable {handler(ec);};
  post(strand,std::move(cb));
}
}
