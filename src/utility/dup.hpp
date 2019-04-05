#pragma once

#include <boost/asio/ip/tcp.hpp>

#include <boost/asio/spawn.hpp>
#include <usings.hpp>
#include <boost/asio/write.hpp>

namespace msocks
{
template <typename Transform>
error_code pair(
  ip::tcp::socket &src,
  ip::tcp::socket &dst,
  yield_context yield,
  std::vector<uint8_t> &buf,
  Transform transform
)
{
  error_code ec;
  while (true)
  {
    auto n_read = src.async_read_some(buffer(buf),yield[ec]);
    if (ec) goto out;
    transform(buffer(buf,n_read));
    async_write(dst,buffer(buf,n_read),yield[ec]);
    if (ec) goto out;
  }
  out:
  return ec;
}


}