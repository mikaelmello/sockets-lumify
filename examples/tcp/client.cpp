#include <bits/stdc++.h>
#include "sockets.hpp"

using namespace std;

int main() {
  Socket::TCPSocket client;

  try {
    client.connect("localhost", 8080);
    cout << "Connected to server" << endl;
    try {
      for (;;) {
        string op, data;
        cin >> op;
        if (op == "recv") {
          std::cout << "Receiving data" << std::endl;
          std::cout << client.recv(256) << std::endl;
        }
        else {
          cin >> data;
          std::cout << "Sending data: " << data << std::endl;
          client.send(data);
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