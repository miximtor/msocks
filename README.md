# msocks
For scientific internet surfing

[![Build Status](https://travis-ci.org/miximtor/msocks.svg?branch=master)](https://travis-ci.org/miximtor/msocks)


###dependencies:

  [Boost](https://www.boost.org/) > 1.69
  
    required boost components: 
        system 
        coroutine
        context
        asio
        random
        
  [libbotan](https://botan.randombit.net/) > 2.10.0

  [spdlog](https://github.com/gabime/spdlog) (header only)
  
###How to build

```
git clone https://github.com/miximtor/msocks.git
cd msocks
git submodule init
git submodule update
cmake . -DCMAKE_BUILD_TYPE=Release
make -j8 && make install
```

###How to run

Run msocks as server:

`
msocks s <server-ip> <server-port> <encrypt-key> <speed limit>
`

Or run msocks as client:

`
msocks c <server-ip> <server-port> <encrypt-key> 
`

###Todo

1) add systemd config

2) add udp forwarding

3) add multiple encrypt strategy