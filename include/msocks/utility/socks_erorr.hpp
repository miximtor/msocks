#pragma once
#include <boost/system/error_code.hpp>

namespace msocks
{
namespace errc
{
enum errc_t
{
  success = 0,
  version_not_supported = 1,
  auth_not_found = 2,
  cmd_not_supported = 3,
  address_not_supported = 4
};

std::string make_errc_string(errc_t code)
{
  switch ( code )
  {
    case success:
      return "success";
    case version_not_supported:
      return "unsupported socks version";
    case auth_not_found:
      return "anonymous auth method not found";
    case cmd_not_supported:
      return "connect command not supported";
    case address_not_supported:
      return "address type not supported";
    default:
      return "unknown";
  }
}

const char *
make_errc_string(errc_t code, char *buffer, std::size_t len) noexcept
{
  switch ( code )
  {
    case success:
      return strncpy(buffer, "success", len);
    case version_not_supported:
      return strncpy(buffer, "unsupported socks version", len);
    case auth_not_found:
      return strncpy(buffer, "anonymous auth method not found", len);
    case cmd_not_supported:
      return strncpy(buffer, "connect command not supported", len);
    case address_not_supported:
      return strncpy(buffer, "address type not supported", len);
    default:
      return strncpy(buffer, "unknown", len);
  }
}

bool code_in_range(int code)
{
  return 0 <= code && code < 5;
}


}

namespace detail
{
class socks_category : public boost::system::error_category
{
public:
  socks_category() = default;
  
  socks_category(const socks_category &) = delete;
  
  socks_category &operator=(const socks_category &) = delete;
  
  const char *name() const noexcept override
  {
    return "msocks::socks";
  }
  
  std::string message(int ev) const override
  {
    return errc::make_errc_string(errc::errc_t(ev));
  }
  
  const char *
  message(int ev, char *buffer, std::size_t len) const noexcept override
  {
    return errc::make_errc_string(errc::errc_t(ev), buffer, len);
  }
  
  boost::system::error_condition
  default_error_condition(int ev) const noexcept override
  {
    return boost::system::error_condition(ev, *this);
  }
  
  bool equivalent(int code,
                  const boost::system::error_condition &condition) const noexcept override
  {
    if ( condition.category() != *this )
      return false;
    return errc::code_in_range(code);
  }
  
  bool equivalent(const boost::system::error_code &code,
                  int condition) const noexcept override
  {
    if ( code.category() != *this )
      return false;
    return errc::code_in_range(condition);
  }
  
};
}

boost::system::error_category &socks_category()
{
  static detail::socks_category category;
  return category;
}

}

namespace boost::system
{
template<>
struct is_error_code_enum<msocks::errc::errc_t> : public std::true_type
{
};
}
