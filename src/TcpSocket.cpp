#include "TcpSocket.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>


TcpSocket::TcpSocket() {
    socket_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_desc == -1) {
        perror("socket failed ");
        return;
    }
    int num = 1;
    int keepidle = 3;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &num, sizeof(int)) == -1)
        print_error("setsockopt(SO_REUSEADDR) failed ");

    if (setsockopt(socket_desc, SOL_SOCKET, SO_KEEPALIVE, &num, sizeof(int)) == -1)
        print_error("setsockopt(SO_KEEPALIVE) failed ");
    if (setsockopt(socket_desc, SOL_TCP, TCP_KEEPINTVL, &keepidle, sizeof(int)) == -1)
        print_error("setsockopt(TCP_KEEPINTVL) failed ");

}


TcpSocket::TcpSocket(int descriptor) {
    socket_desc = descriptor;
    int num = 1;
    int keepidle = 3;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_KEEPALIVE, &num, sizeof(int)) == -1)
        print_error("setsockopt(SO_KEEPALIVE) failed ");
    if (setsockopt(socket_desc, SOL_TCP, TCP_KEEPINTVL, &keepidle, sizeof(int)) == -1)
        print_error("setsockopt(TCP_KEEPINTVL) failed ");
}


bool TcpSocket::connect(const std::string& ip_address, uint16_t port)
{
    sockaddr_in *server = (sockaddr_in*)calloc(sizeof(sockaddr_in), 1);
    server->sin_family = AF_INET;
    server->sin_port = htons(port);
    server->sin_addr.s_addr = inet_addr(ip_address.c_str());

    if (::connect(socket_desc, (const sockaddr*)server, sizeof(sockaddr_in)) == -1)
    {
        print_error("connect failed ");
        return false;
    }

    return true;
}


bool TcpSocket::bind(uint16_t port)
{
    sockaddr_in server;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    if(::bind(socket_desc, (sockaddr*)&server, sizeof(sockaddr_in)) < 0)
    {
        print_error("bind failed ");
        return false;
    }
    return true;
}


bool TcpSocket::listen(int numConnections)
{
    if (::listen(socket_desc, numConnections) == -1){
        print_error("listen failed ");
        return false;
    }
    return true;
}


int TcpSocket::accept()
{
    socklen_t c = sizeof(struct sockaddr_in);
    sockaddr_in client;
    int new_socket;
    do {
        new_socket = ::accept(socket_desc, (struct sockaddr *)&client, &c);
        if (new_socket == -1 && errno != EINTR) {
            print_error("accept failed ");
            return 0;
        }
    } while(new_socket == -1);
    return new_socket;
}


ssize_t TcpSocket::send(cstring message, int flags)
{
    ssize_t tr;
    if((tr = ::send(socket_desc, message.c_str(), message.size(), flags)) == -1)
    {
        return 0;
    }
    return tr;
}

std::string TcpSocket::recv()
{
    char buffer[BUF_SIZE + 1];
    int res;
    if ((res = ::recv(socket_desc, buffer, BUF_SIZE, 0)) == -1)
    {
        print_error("recv failed ");
        return "";
    }
    if (res == 0)
        return "";
    buffer[res] = 0;
    return buffer;
}


void TcpSocket::close() {
    ::close(socket_desc);
}

void TcpSocket::shutdown() {
    ::shutdown(socket_desc, SHUT_RDWR);
}

int TcpSocket::recv_to_buffer(char *buffer, int size) {
    return ::recv(socket_desc, buffer, size, 0);
}


int TcpSocket::send_file(int file_desc, ssize_t size, off_t offset) {
    off_t offs = offset;
    return sendfile(socket_desc, file_desc, &offs, size);
}
