#include "tcpsocket.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>


TcpSocket::TcpSocket() {
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("Creating socket failed");
        return;
    }
}


TcpSocket::TcpSocket(int descriptor) {
    socket_desc = descriptor;
}


bool TcpSocket::connect(const std::string& ip_address, uint16_t port)
{
    sockaddr_in *server = (sockaddr_in*)calloc(sizeof(sockaddr_in), 1);
    server->sin_family = AF_INET;
    server->sin_port = htons(port);
    server->sin_addr.s_addr = inet_addr(ip_address.c_str());

    int res;
    if ((res = ::connect(socket_desc, (const sockaddr*)server, sizeof(sockaddr_in))) < 0)
    {
        perror("connect failed");
        return false;
    }

    return true;
}


bool TcpSocket::bind(uint16_t port)
{
    sockaddr_in server;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    server.sin_family = AF_INET;

    if(::bind(socket_desc, (sockaddr*)&server, sizeof(sockaddr_in)) < 0)
    {
        perror("bind failed");
        return false;
    }
    return true;
}


bool TcpSocket::listen(int numConnections)
{
    if (::listen(socket_desc, numConnections) == -1){
        perror("listen failed");
        return false;
    }
    return true;
}


int TcpSocket::accept()
{
    socklen_t c = sizeof(struct sockaddr_in);
    sockaddr_in client;
    int new_socket;
    do{
        new_socket = ::accept(socket_desc, (struct sockaddr *)&client, &c);
        if (new_socket == -1 && errno != EINTR)
        {
            perror("accept failed");
            return 0;
        }
    }while(new_socket == -1);
    return new_socket;
}


ssize_t TcpSocket::send(const std::string& message)
{
    ssize_t tr;
    if((tr = ::send(socket_desc, message.c_str(), message.size(), 0)) == -1)
    {
        perror("send failed");
        return 0;
    }
    return tr;
}

std::string TcpSocket::recv(int BUF_SIZE)
{
    char* buffer = new char[BUF_SIZE];
    if (::recv(socket_desc, buffer, BUF_SIZE, 0) == -1)
    {
        perror("read failed");
        return "";
    }
    return buffer;
}


void TcpSocket::close(){
    shutdown(socket_desc, SHUT_RDWR);
}
