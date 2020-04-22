//
// Created by grigoriy on 19.2.20.
//
#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H
#include <string>


class TcpSocket
{
public:
    TcpSocket();

    TcpSocket(int descriptor);

    bool connect(const std::string& ip_address, uint16_t port);

    bool bind(uint16_t port);

    bool listen(int numConnections = 1000);

    int accept();

    ssize_t send(const std::string& message);

    std::string recv(int size);

    void close();

private:
    int socket_desc;
    bool server;
};
#endif // TCP_SOCKET_H
