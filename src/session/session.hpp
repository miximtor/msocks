#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <botan/stream_cipher.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <usings.hpp>

namespace msocks
{

struct session : public noncopyable
{
public:
  session(
    io_context::strand &strand_,
    ip::tcp::socket local_,
    const std::vector<uint8_t> &key_
    ) :
    strand(strand_),
    local(std::move(local_)),
    remote(local.get_executor().context()),
    session_uuid(boost::uuids::to_string(boost::uuids::random_generator_mt19937()())),
    key(key_)
  {}
protected:
  
  void cipher_setup(
    const std::string &method,
    const std::vector<uint8_t> &iv,
    std::unique_ptr<Botan::StreamCipher> &cipher
    )
  {
    cipher = Botan::StreamCipher::create_or_throw(method);
    cipher->set_key(key.data(),key.size());
    cipher->set_iv(iv.data(),iv.size());
  }
  std::string session_uuid;
  io_context::strand &strand;
  ip::tcp::socket local;
  std::array<uint8_t,4096> buffer_local;
  ip::tcp::socket remote;
  std::array<uint8_t,4096> buffer_remote;
  
  const std::vector<uint8_t> &key;
  std::unique_ptr<Botan::StreamCipher> send_cipher;
  std::unique_ptr<Botan::StreamCipher> recv_cipher;
};

}