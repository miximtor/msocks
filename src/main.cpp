#include <vector>
#include <string>
#include <endpoint/server.hpp>
#include <endpoint/client.hpp>
#include <cryptopp/sha.h>

int main(int argc, char *argv[])
{
  using namespace std::string_literals;
  std::string ip = argv[2];
  unsigned short port = std::stoi(argv[3]);
  ip::tcp::endpoint listen(ip::make_address_v4(ip), port);
  std::string password = argv[4];
  SHA256 sha256;
  SecByteBlock key((SHA256::DIGESTSIZE));
  sha256.CalculateDigest(key.data(), (byte *) password.data(), password.size());
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