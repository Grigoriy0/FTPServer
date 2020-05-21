

#include "Client.h"
#include "TcpSocket.h"
#include "DataThread.h"


Client::Client(TcpSocket *cmdSock) {
    active = false;
    _type = ASCII;
    cmdSocket = cmdSock;
    dt = nullptr;
    dt_info = nullptr;
}

bool Client::send_command(const std::string &command) {
    return write(pipe[1], command.c_str(), command.size()) != -1;
}

Client::~Client() {
    delete fe;
    delete dt;
    delete dt_info;
    delete cmd;
    delete cmdSocket;
}