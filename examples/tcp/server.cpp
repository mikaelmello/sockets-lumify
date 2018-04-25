#include <bits/stdc++.h>
#include "sockets.hpp"

using namespace std;

int main() {
  Socket::TCPSocket server;

  try {
    // Binds the server to port 8080
    server.bind(8080);

    // Listens for incoming connections
    server.listen();

    // Blocks until it accepts a client
    std::shared_ptr<Socket::TCPSocket> client = server.accept();
    cout << "Connected client" << endl;
    try {
      for (;;) {
        string op, data;
        cin >> op >> data;
        if (op == "recv") {
          std::cout << "Receiving data" << std::endl;
          std::cout << client->recv(256) << std::endl;
        }
        else {
          std::cout << "Sending data: " << data << std::endl;
          client->send(data);
        }
      }
    }
    catch (const Socket::ClosedConnection & x) {
      cout << "Connection closed" << endl;
    }
  }
  catch(std::exception &e) {
    std::cout << e.what() << std::endl;
  }
}