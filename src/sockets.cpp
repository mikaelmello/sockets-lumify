#include "../include/sockets.hpp"

#include <iostream>

using namespace Socket;

BaseSocket::BaseSocket(int type, int flags) {

    // Inicializa socketInfo
    socketInfo = new addrinfo;
    std::memset(socketInfo, 0, sizeof(addrinfo));

    // Prenche a estrutura de endereço do socket, sempre IPv4
    socketInfo->ai_family = AF_INET;
    socketInfo->ai_socktype = type;
    socketInfo->ai_flags = flags;

    // Mesma família (IPv4) e tipo de socket da socketInfo
    socketFd = socket(AF_INET, type, 0);
    if (socketFd == -1) {
        throw SocketException("Could not create socket. Error: " + std::string(strerror(errno)));
    }

    // Permite reuso de endereço (evita "Address already in use")
    int optval = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

BaseSocket::BaseSocket(int socket, int type, addrinfo sInfo, uint32_t port, bool isBound) :
    socketFd(socket), portUsed(port), isBound(isBound) {

    // Verifica que o endereço é IPv4 e do mesmo tipo definido pelo parâmetro
    if (sInfo.ai_family != AF_INET || sInfo.ai_socktype != type) {
        throw SocketException("sInfo passed is not defined as the defined type and protocol IPv4");
    }

    // Copia as inforamações do sInfo (sockaddr).
    socketInfo = new addrinfo(sInfo);

    // Permite reuso de endereço (evita "Address already in use")
    int optval = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

BaseSocket::~BaseSocket() {
    // Libera memória e encerra o socket
    if (socketInfo) freeaddrinfo(socketInfo);
    close();
}

void BaseSocket::bind(uint32_t port) {

    // Não é possivel vincular um socket que já está vinculado
    // Logo se a porta desejada for a mesma que já está vinculada, nada acontece.
    // Uma exceção é lançada caso contrário
    if (isBound && portUsed != port) {
        throw ConnectionException("Socket is already bound to port " 
            + std::to_string(portUsed));
    }
    else if (isBound && portUsed == port) {
        return;
    }

    // Pela documentação:
    //     If the AI_PASSIVE bit is set it indicates that the
    // returned socket address structure is intended for use
    // in a call to bind(2). In this case, if the hostname 
    // argument is the null pointer, then the IP address portion
    // of the socket address structure will be set to INADDR_ANY 
    // for an IPv4 address
    socketInfo->ai_flags |= AI_PASSIVE;
    getAddrInfo("NULL", port);


    int result;
    int totalIPv4Addrs = 0; // Conta quandos endereços IPv4 foram encontrados
    addrinfo * newAddrInfo;

    for (newAddrInfo = socketInfo; newAddrInfo; newAddrInfo = newAddrInfo->ai_next) {

        // Ignora qualquer endereço não IPv4
        if (newAddrInfo->ai_family == AF_INET) totalIPv4Addrs++;
        else continue;

        // Tenta vincular a qualquer endereço IPv4 encontrado.
        result = ::bind(socketFd, newAddrInfo->ai_addr, newAddrInfo->ai_addrlen);
        if (result == -1) continue;

        // Se a operação é bem-sucedida, seta as variáveis e retorna
        portUsed = port;
        isBound = true;
        return;
    }

    // Se nenhum endereço IPv4 é lançado, lança exceção
    if (totalIPv4Addrs == 0) {
        throw ConnectionException("Could not find IPv4 addresses to bind on port " 
            + std::to_string(port) + ".");
    }

    // Lança exceção caso não seja possível vincular a nenhum IPv4 encontrado.
    throw ConnectionException("Could not bind to port " + std::to_string(port) 
            + ". Error: " + std::string(strerror(errno)));
}

void BaseSocket::close() {

    // Se a socket não foi setada ou já foi fechada, nada acontece.
    if (socketFd == -1 || isClosed) return;

    int result = ::close(socketFd);
    if (result == -1) {
        throw SocketException("Could not close socket. Error: " + 
            std::string(strerror(errno)));
    }

    isBound = false;
    isClosed = true;
    portUsed = -1;
    socketFd = -1;

}

void BaseSocket::getAddrInfo(const std::string address, uint32_t port) {

    addrinfo   hints = *socketInfo;
    addrinfo * newSocketInfo;
    const char * cAddress;

    // cAddress = NULL significa que o getaddrinf() buscará por endereços
    // locais da maquina
    if (address == "NULL") cAddress = NULL;
    else cAddress = address.c_str();

    int result = getaddrinfo(cAddress, std::to_string(port).c_str(), &hints, &newSocketInfo);

    if (result != 0) {
        throw ConnectionException("Error on getaddrinfo(): " + 
            std::string(gai_strerror(result)));
    }

    // Libera a memoria da socketInfo atual para ela ser realocada por getaddrinfo()
    if (socketInfo) freeaddrinfo(socketInfo);
    socketInfo = newSocketInfo;  
}

bool BaseSocket::validateIpAddress(const std::string &ipAddress) {
    sockaddr_in sa;
    int result = ::inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
    return result != 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                  TCPSOCKET
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

TCPSocket::TCPSocket(int flags) : BaseSocket(SOCK_STREAM, flags) {}

TCPSocket::TCPSocket(int socket, addrinfo socketInfo, uint32_t portUsed, bool isBound, bool isListening, bool isConnected) : 
    BaseSocket(socket, SOCK_STREAM, socketInfo, portUsed, isBound), isListening(isListening), isConnected(isConnected) {
}

void TCPSocket::connect(const std::string& address, uint32_t port) {

    if (isListening) {
        throw SocketException("Socket is already listening incoming connections on port " 
                                + std::to_string(portUsed));
    }
    if (isConnected) {
        throw SocketException("Socket is already connected to an address");
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
        
void TCPSocket::listen(uint32_t backlog) {

    if (!isBound) {
        throw SocketException("Can not listen when socket is not bound to any port");
    }
    if (isListening) {
        throw SocketException("Socket is already listening incoming connections on port " 
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
        throw SocketException("Socket is not listening to incoming connections to accept one");
    }

    sockaddr clientAddress;
    socklen_t sin_size = sizeof(sockaddr);
    int clientFd = ::accept(socketFd, &clientAddress, &sin_size);

    if (clientFd == -1) {
        throw ConnectionException("Error while accepting a connection. Error: " 
            + std::string(strerror(errno)));
    }

    addrinfo clientInfo;
    memset(&clientInfo, 0, sizeof(addrinfo));

    clientInfo.ai_family = AF_INET;
    clientInfo.ai_socktype = SOCK_STREAM;
    clientInfo.ai_addr = &clientAddress;

    return TCPSocket(clientFd, clientInfo, portUsed, false, false, true);

}
        
void TCPSocket::send(const std::string& message, int flags) {
    send((uint8_t *) message.c_str(), message.length() + 1, flags);
}
        
void TCPSocket::send(const uint8_t* message, int length, int flags) {

    if (isListening || !isConnected) {
        throw SocketException("Can't send message from a socket that is not connected");
    }

    int bytesLeft = length;
    int bytesSent = 0;
    const uint8_t* cMessageLeft;

    while (bytesSent < length) {

        cMessageLeft = message + bytesSent;

        int result = ::send(socketFd, cMessageLeft, bytesLeft, flags | MSG_NOSIGNAL);
        if (result == -1) {
            throw ConnectionException("Could not send message. Error: " 
                + std::string(strerror(errno)));
        }

        bytesLeft -= result;
        bytesSent += result;

    }

}

std::string TCPSocket::recv(uint64_t maxlen, int flags) {

    uint8_t* message = recv(&maxlen, flags);

    std::string stringMessage((char *) message);
    free(message);
    return stringMessage;

}

uint8_t* TCPSocket::recv(uint64_t* length, int flags) {

    if (isListening || !isConnected) {
        throw SocketException("Can't receive message in a socket that is not connected");
    }

    uint64_t maxlen = *length;

    uint8_t buffer[maxlen];
    int result = ::recv(socketFd, buffer, maxlen, flags);

    if (result == -1) {
        throw ConnectionException("Could not receive message. Error: " 
            + std::string(strerror(errno)));
    }
    if (result ==  0) {
        isConnected = false;
        throw ClosedConnection("Client closed connection.");
    }

    uint8_t* message = (uint8_t *) malloc(result);
    memcpy(message, buffer, result);
    *length = result;

    return message;

}

void TCPSocket::close() {

    BaseSocket::close();

    isConnected = false;
    isListening = false;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                  UDPRECV
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

UDPRecv::UDPRecv(const std::string& name, const std::string& address, const std::string& msg, uint32_t port) :
                 name(name), address(address), msg(msg), port(port) {
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *                  UDPSOCKET
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

UDPSocket::UDPSocket(int flags) : BaseSocket(SOCK_DGRAM, flags) {}

void UDPSocket::sendto(const std::string& address, uint32_t port, 
    const std::string& message, int flags) {

    int msgLength = message.length();
    int bytesLeft = msgLength;
    int bytesSent = 0;
    const char* cMessage = message.c_str();

    socklen_t serverLength;
    sockaddr_in serverAddress;
    hostent *server;

    /* build the server's Internet address */
    ::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;

    if (!validateIpAddress(address)) {
        server = ::gethostbyname(address.c_str());
        if (server == NULL) {
            throw ConnectionException("No such host as " + address);
        }
        ::memcpy(server->h_addr, &serverAddress.sin_addr.s_addr, server->h_length);

    }
    else {
        ::inet_pton(AF_INET, address.c_str(), &(serverAddress.sin_addr));
    }

    serverAddress.sin_port = ::htons(port);
    serverLength = sizeof(serverAddress);

    while (bytesSent < msgLength) {

        const char* cMessageLeft = cMessage + bytesSent;

        int result = ::sendto(socketFd, cMessageLeft, bytesLeft, flags | MSG_NOSIGNAL, 
                             (sockaddr*) &serverAddress, serverLength);
        if (result == -1) {
            throw ConnectionException("Could not send message. Error: " 
                + std::string(strerror(errno)));
        }

        bytesLeft -= result;
        bytesSent += result;

    }

}

UDPRecv UDPSocket::recvfrom(uint64_t maxlen, int flags) {
    
    sockaddr_in clientAddress;
    hostent* clientInfo;
    socklen_t clientLength = sizeof(clientAddress);

    char buffer[maxlen+1];
    int result = ::recvfrom(socketFd, buffer, maxlen, flags, (sockaddr *) &clientAddress, &clientLength);

    if (result == -1) {
        throw ConnectionException("Could not receive message. Error: " 
            + std::string(strerror(errno)));
    }
    buffer[result] = '\0';

    clientInfo = ::gethostbyaddr((const char *)&clientAddress.sin_addr.s_addr, 
                               sizeof(clientAddress.sin_addr.s_addr), AF_INET);

    std::string hostName(clientInfo->h_name);
    std::string hostAddress(::inet_ntoa(clientAddress.sin_addr));
    std::string message(buffer);

    int port = ::ntohs(clientAddress.sin_port);

    return UDPRecv(hostName, hostAddress, message, port);


}

