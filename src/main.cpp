#include <vector>
#include <string>
#include <endpoint/server.hpp>
#include <endpoint/client.hpp>
#include <botan/sha2_32.h>
#include <glog/logging.h>
int main(int argc, char *argv[])
{
  google::InitGoogleLogging(argv[0]);
  google::SetStderrLogging(google::GLOG_INFO);
  using namespace std::string_literals;
  std::string ip = argv[2];
  unsigned short port = std::stoi(argv[3]);
  ip::tcp::endpoint listen(ip::make_address_v4(ip), port);
  std::string password = argv[4];
  Botan::SHA_256 sha256;
  sha256.update(password);
  std::vector<uint8_t> key;
  sha256.final(key);
  if ( argv[1] == "s"s )
  {
    msocks::server server(listen, key);
    server.start();
  }
  else if ( argv[1] == "c"s )
  {
    ip::tcp::endpoint local_ep(ip::make_address("127.0.0.1"), 1081);
    msocks::client client(local_ep, listen, key);
    client.start();
  }
  
}