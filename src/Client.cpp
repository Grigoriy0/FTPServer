

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

bool Client::send_command(cstring command) {
    return write(_pipe[1], command.c_str(), command.size()) != -1;
}

Client::~Client() {
    delete fe;
    delete dt;
    delete dt_info;
    delete cmd;
    delete cmdSocket;
}


std::string Client::init_active_thread(cstring arg) {
    if (pipe(_pipe) == -1) {
        print_error("E: pipe failed");
        return "550 Error on the server\r\n";
    }
    dt_info = new DataThread(cmdSocket, fe, _pipe);
    // start_active thread
    dt = new Thread<void, DataThread *, std::string, bool>{DataThread::run, dt_info, arg, true};
    return "";
}

std::string Client::init_passive_thread(cstring arg) {
    if (pipe(_pipe) == -1) {
        print_error("E: _pipe failed");
        return "550 Error on the server\r\n";
    }
    dt_info = new DataThread(cmdSocket, fe, _pipe);
    // start_passive thread
    dt = new Thread<void, DataThread *, std::string, bool>{DataThread::run, dt_info, arg, false};
    return "";
}
