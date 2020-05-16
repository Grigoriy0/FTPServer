
#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H
#include <string>
#include "defines.h"

class TcpSocket
{
public:
    TcpSocket();

    explicit TcpSocket(int descriptor);

    bool connect(const std::string& ip_address, uint16_t port);

    bool bind(uint16_t port);

    bool listen(int numConnections = 1000);

    int accept();

    ssize_t send(cstring message, int flags = 0);

    std::string recv();

    int recv_to_buffer(char* buffer, int size = BUF_SIZE);

    void close();

    void shutdown();

 private:
    int socket_desc;
};
#endif // TCP_SOCKET_H
