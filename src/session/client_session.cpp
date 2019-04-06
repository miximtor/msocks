#include "client_session.hpp"

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/endian/conversion.hpp>

#include <glog/logging.h>

#include <botan/auto_rng.h>

#include <utility/socks_constants.hpp>
#include <utility/socket_pair.hpp>

namespace msocks
{

bool client_session::local_socks_auth(yield_context yield)
{
  struct __attribute__((packed))
  {
    uint8_t version;
    uint8_t n_method;
  } auth_method{};
  
  async_read(
    local,
    buffer(&auth_method, sizeof(auth_method)),
    transfer_exactly(sizeof(auth_method)),
    yield
  );
  
  std::vector<uint8_t> methods_buf;
  async_read(
    local,
    dynamic_buffer(methods_buf),
    transfer_exactly(auth_method.n_method),
    yield
  );
  
  bool find_auth = false;
  for ( auto method : methods_buf )
  {
    if ( method == constant::AUTH_NO_AUTH )
    {
      find_auth = true;
      break;
    }
  }
  
  if ( !find_auth )
  {
    return false;
  }
  
  struct __attribute__((packed))
  {
    uint8_t version;
    uint8_t method;
  } auth_reply{constant::SOCKS5_VERSION, constant::AUTH_NO_AUTH};
  
  async_write(
    local,
    buffer(&auth_reply, sizeof(auth_reply)),
    transfer_exactly(sizeof(auth_reply)),
    yield
  );
  return true;
}

std::pair<bool, std::string>
client_session::get_host_service(yield_context yield)
{
  struct __attribute__((packed))
  {
    uint8_t version;
    uint8_t cmd;
    uint8_t reserve;
    uint8_t addr_type;
  } request{};
  
  async_read(
    local,
    buffer(&request, sizeof(request)),
    transfer_exactly(sizeof(request)),
    yield
  );
  
  if ( request.version != constant::SOCKS5_VERSION )
  {
    return {false, ""};
  }
  
  switch ( request.cmd )
  {
    case constant::CONN_TCP:
      break;
    case constant::CONN_BND:
    case constant::CONN_UDP:
    default:
      LOG(INFO) << "unsupported cmd: " << request.cmd;
      return {false, ""};
  }
  
  std::string a_p;
  std::vector<uint8_t> ap_buf;
  if ( request.addr_type == constant::ADDR_IPV4 )
  {
    async_read(
      local,
      dynamic_buffer(ap_buf),
      transfer_exactly(32 / 8 + 2),
      yield
    );
    
    struct __attribute__((packed))
    {
      uint32_t addr;
      uint16_t port;
    } addr_pt{};
    
    buffer_copy(
      buffer(&addr_pt, sizeof(addr_pt)),
      buffer(ap_buf),
      sizeof(addr_pt)
    );
    a_p = ip::make_address_v4(big_to_native(addr_pt.addr)).to_string();
    a_p += ":";
    a_p += std::to_string(big_to_native(addr_pt.port));
  }
  else if ( request.addr_type == constant::ADDR_IPV6 )
  {
    async_read(
      local,
      dynamic_buffer(ap_buf),
      transfer_exactly(128 / 8 + 2),
      yield
    );
    
    struct __attribute__((packed))
    {
      ip::address_v6::bytes_type addr;
      uint16_t port;
    } addr_pt{};
    
    buffer_copy(
      buffer(&addr_pt, sizeof(addr_pt)),
      buffer(ap_buf),
      sizeof(addr_pt)
    );
    std::reverse(addr_pt.addr.begin(), addr_pt.addr.end());
    a_p = ip::make_address_v6(addr_pt.addr).to_string();
    a_p += ":";
    a_p += std::to_string(big_to_native(addr_pt.port));
  }
  else if ( request.addr_type == constant::ADDR_DOMAIN )
  {
    uint8_t domain_length;
    async_read(
      local,
      buffer(&domain_length, sizeof(domain_length)),
      transfer_exactly(sizeof(domain_length)),
      yield
    );
    
    async_read(
      local,
      dynamic_buffer(ap_buf),
      transfer_exactly(domain_length + 2),
      yield
    );
    std::copy(ap_buf.begin(), ap_buf.begin() + domain_length,
              std::back_inserter(a_p));
    a_p += ":";
    uint16_t port = 0;
    std::copy(ap_buf.begin() + domain_length, ap_buf.end(), (uint8_t *) &port);
    big_to_native_inplace(port);
    a_p += std::to_string(port);
  }
  else
  {
    return {false, ""};
  }
  
  struct __attribute__((packed))
  {
    uint8_t version;
    uint8_t rep;
    uint8_t rsv;
    uint8_t atyp;
    uint32_t bnd_addr;
    uint16_t bnd_port;
  } reply{
    constant::SOCKS5_VERSION,
    0x00,
    0x00,
    constant::ADDR_IPV4,
    0x00000000,
    0x0000
  };
  
  async_write(
    local,
    buffer(&reply, sizeof(reply)),
    transfer_all(),
    yield
  );
  return {true, std::move(a_p)};
}

std::pair<bool, std::string>
client_session::do_local_socks5(yield_context yield)
{
  if ( local_socks_auth(yield))
  {
    return get_host_service(yield);
  }
  else
  {
    return {false, ""};
  }
}

void client_session::start(yield_context yield)
{
  try
  {
    auto[success, addr_port] = do_local_socks5(yield);
    if ( !success )
    {
      LOG(WARNING) << "do local socks5 failed";
      return;
    }
    LOG(INFO) << "connect to: " << addr_port;
    remote.async_connect(addr, yield);
    handshake(yield, std::move(addr_port));
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
    LOG(WARNING) << __LINE__ << e.what();
    return;
  }
  
}

client_session::client_session(
  io_context::strand &strand_,
  ip::tcp::socket local_,
  const ip::tcp::endpoint &addr_,
  const std::vector<uint8_t> &key_) :
  strand(strand_),
  local(std::move(local_)),
  remote(local.get_executor().context()),
  addr(addr_),
  key(key_)
{
}


void client_session::handshake(yield_context yield, std::string host_service)
{
  Botan::AutoSeeded_RNG rng;
  std::vector<uint8_t> send_iv(8);
  std::vector<uint8_t> recv_iv(8);
  rng.randomize(send_iv.data(), send_iv.size());
  
  send_cipher = Botan::StreamCipher::create_or_throw("ChaCha(20)");
  send_cipher->set_key(key.data(), key.size());
  send_cipher->set_iv(send_iv.data(), send_iv.size());
  
  uint16_t size = send_iv.size() + host_service.size();
  std::vector<uint8_t> temp(host_service.begin(), host_service.end());
  send_cipher->encrypt(temp);
  
  std::vector<const_buffer> buffers;
  buffers.emplace_back(buffer(&size, sizeof(size)));
  buffers.emplace_back(buffer(send_iv));
  buffers.emplace_back(buffer(temp));
  
  async_write(remote, buffers, transfer_all(), yield);
  async_read(remote, buffer(recv_iv), transfer_all(), yield);
  
  recv_cipher = Botan::StreamCipher::create_or_throw("ChaCha(20)");
  recv_cipher->set_key(key.data(), key.size());
  recv_cipher->set_iv(recv_iv.data(), recv_iv.size());
}

void client_session::do_forward_remote_to_local(yield_context yield)
{
  std::vector<uint8_t> buf(128);
  auto ec = pair(remote, local, yield, buf, [this](mutable_buffer b)
  {
    recv_cipher->cipher1((uint8_t *)b.data(),b.size());
  });
  LOG(INFO) << ec.message();
  local.close();
  remote.close();
}

void client_session::do_forward_local_to_remote(yield_context yield)
{
  std::vector<uint8_t> buf(128);
  auto ec = pair(local, remote, yield, buf, [this](mutable_buffer b)
  {
    send_cipher->cipher1((uint8_t *)b.data(),b.size());
  });
  LOG(INFO) << ec.message();
  local.close();
  remote.close();
}

void client_session::go()
{
  spawn(strand, [this, p = shared_from_this()](yield_context yield)
  {
    start(yield);
  });
}

}
