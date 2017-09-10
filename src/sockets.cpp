#include "../include/sockets.hpp"

using namespace Socket;

GeneralSocket::GeneralSocket(int type, int flags) {
    socketInfo = new addrinfo;

    std::memset(socketInfo, 0, sizeof(addrinfo));

    socketInfo->ai_family = AF_INET;
    socketInfo->ai_socktype = type;
    socketInfo->ai_flags = flags;

    // Same family and socktype as socketInfo
    socketFd = socket(AF_INET, type, 0);
    if (socketFd == -1) {
        throw ConnectionException("Could not create socket. Error: " + std::string(strerror(errno)));
    }

    // allow reuse of address (to avoid "Address already in use")
    int optval = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

GeneralSocket::GeneralSocket(int socket, int type, addrinfo sInfo, int port, bool isBound) :
                         socketFd(socket), portUsed(port), isBound(isBound) {


    if (sInfo.ai_family != AF_INET || sInfo.ai_socktype != type) {
        throw ConnectionException("sInfo passed is not defined as the defined type and protocol IPv4");
    }

    socketInfo = new addrinfo(sInfo);

    // allow reuse of address (to avoid "Address already in use")
    int optval = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

GeneralSocket::~GeneralSocket() {
    freeaddrinfo(socketInfo);
    close();
}

void GeneralSocket::bind(unsigned int port) {

    if (isBound) throw ConnectionException("Socket is already bound to port " + std::to_string(portUsed));

    // Per documentation: 
    //     If the AI_PASSIVE bit is set it indicates that the
    // returned socket address structure is intended for use
    // in a call to bind(2). In this case, if the hostname 
    // argument is the null pointer, then the IP address portion
    // of the socket address structure will be set to INADDR_ANY 
    // for an IPv4 address
    socketInfo->ai_flags |= AI_PASSIVE;
    getAddrInfo("NULL", port);

    int result;
    int totalIPv4Addrs = 0;
    addrinfo * newAddrInfo;

    for (newAddrInfo = socketInfo; newAddrInfo; newAddrInfo = newAddrInfo->ai_next) {

        if (newAddrInfo->ai_family == AF_INET) totalIPv4Addrs++;
        else continue;

        result = ::bind(socketFd, newAddrInfo->ai_addr, newAddrInfo->ai_addrlen);
        if (result == -1) continue;

        portUsed = port;
        isBound = true;
        return;
    }

    if (totalIPv4Addrs == 0) {
        throw ConnectionException("Could not find IPv4 addresses to bind on port " 
                                + std::to_string(port) + ".");
    }

    throw ConnectionException("Could not bind to port " + std::to_string(port) 
                            + ". Error: " + std::string(strerror(errno)));
}

void GeneralSocket::close() {

    if (socketFd == -1 || isClosed) return;

    int result = ::close(socketFd);
    if (result == -1) {
        throw ConnectionException("Could not close socket. Error: " + std::string(strerror(errno)));
    }

    isBound = false;

    portUsed = -1;
    socketFd = -1;


}

void GeneralSocket::getAddrInfo(const std::string address, unsigned int port) {

    addrinfo hints = *socketInfo;
    const char * cAddress;

    if (address == "NULL") cAddress = NULL;
    else cAddress = address.c_str();

    int result = getaddrinfo(cAddress, std::to_string(port).c_str(), &hints, &socketInfo);


    if (result != 0) {
        throw ConnectionException("Error on getaddrinfo(): " + std::string(gai_strerror(result)));
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                  TCPSOCKET
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

TCPSocket::TCPSocket(int flags) : GeneralSocket(SOCK_STREAM, flags) {}

TCPSocket::TCPSocket(int socket, addrinfo socketInfo, int portUsed, bool isBound, bool isListening, bool isConnected) : 
                     GeneralSocket(socket, SOCK_STREAM, socketInfo, portUsed, isBound), isListening(isListening), isConnected(isConnected) {
}

void TCPSocket::connect(const std::string& address, unsigned int port) {

    if (isListening) {
        throw ConnectionException("Socket is already listening incoming connections on port " 
                                + std::to_string(portUsed));
    }
    if (isConnected) {
        throw ConnectionException("Socket is already connected to an address");
    }
    
    getAddrInfo(address, port);

    int result;
    int totalIPv4Addrs = 0;
    addrinfo * newAddrInfo;

    for (newAddrInfo = socketInfo; newAddrInfo; newAddrInfo = newAddrInfo->ai_next) {

        if (newAddrInfo->ai_family == AF_INET) totalIPv4Addrs++;
        else continue;

        result = ::connect(socketFd, newAddrInfo->ai_addr, newAddrInfo->ai_addrlen);
        if (result == -1) continue;

        isConnected = true;
        return;
    }

    if (totalIPv4Addrs == 0) {
        throw ConnectionException("Could not find IPv4 addresses to connect to " + address + 
                                  " on port " + std::to_string(port) + ".");
    }

    throw ConnectionException("Could not connect to address " + address +
                              " on port " + std::to_string(port) + 
                              ". Error: " + std::string(strerror(errno)));

}
        
void TCPSocket::listen(unsigned int backlog) {

    if (!isBound) {
        throw ConnectionException("Can not listen when socket is not bound to any port");
    }
    if (isListening) {
        throw ConnectionException("Socket is already listening incoming connections on port " 
                                + std::to_string(portUsed));
    }

    int result = ::listen(socketFd, backlog);
    if (result == -1) {
        throw ConnectionException("Could not listen on port " + std::to_string(portUsed) 
                                + ". Error: " + std::string(strerror(errno)));
    }

    isListening = true;

}

TCPSocket TCPSocket::accept() {

    if (!isListening) {
        throw ConnectionException("Socket is not listening to incoming connections to accept one");
    }

    sockaddr_in * clientAddress = new sockaddr_in;
    memset(&clientAddress, 0, sizeof(sockaddr_in));
    socklen_t sin_size = sizeof(sockaddr_in);
    int clientFd = ::accept(socketFd, (sockaddr *)&clientAddress, &sin_size);

    if (clientFd == -1) {
        throw ConnectionException("Error while accepting a connection. Error: " + std::string(strerror(errno)));
    }

    addrinfo clientInfo;
    memset(&clientInfo, 0, sizeof(addrinfo));

    clientInfo.ai_family = AF_INET;
    clientInfo.ai_socktype = SOCK_STREAM;
    clientInfo.ai_addr = (sockaddr*)clientAddress;

    return TCPSocket(clientFd, clientInfo, portUsed, false, false, true);

}
        
void TCPSocket::send(const std::string& message, int flags) {

    if (isListening || !isConnected) {
        throw ConnectionException("Can't send message from a socket that is not connected");
    }

    int msgLength = message.length();
    int bytesLeft = msgLength;
    int bytesSent = 0;
    const char* cMessage = message.c_str();

    while (bytesSent < msgLength) {

        const char* cMessageLeft = cMessage + bytesSent;

        int result = ::send(socketFd, cMessageLeft, bytesLeft, flags);
        if (result == -1) {
            throw ConnectionException("Could not send message. Error: " + std::string(strerror(errno)));
        }

        bytesLeft -= result;
        bytesSent += result;

    }

}

std::string TCPSocket::recv(unsigned int maxlen, int flags) {

    if (isListening || !isConnected) {
        throw ConnectionException("Can't receive message in a socket that is not connected");
    }

    char buffer[maxlen+1];
    int result = ::recv(socketFd, buffer, maxlen, flags);

    if (result == -1) {
        throw ConnectionException("Could not receive message. Error: " + std::string(strerror(errno)));
    }
    if (result ==  0) {
        isConnected = false;
        throw ClosedConnection("Client closed connection.");
    }
    buffer[result] = '\0';
    std::string message(buffer);

    return message + " " + std::to_string(result);

}

void TCPSocket::close() {

    GeneralSocket::close();

    isConnected = false;
    isListening = false;


}