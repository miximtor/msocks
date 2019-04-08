#pragma once

#include <string>

namespace msocks
{

struct config
{
  enum Type
  {
    Server = 0,
    Client = 1
  };
  Type type;
  std::string server;
  unsigned short server_port;
  std::string local;
  unsigned short local_port;
  std::string password;
  std::string method;
  std::size_t speed_limit;
};

}