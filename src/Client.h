
#ifndef MASTER_SRC_CLIENT_H_
#define MASTER_SRC_CLIENT_H_
class TcpSocket;
class DataThread;
class FileExplorer;
#include "Thread.h"
#include "sys/signal.h"

struct Client {
    Client(TcpSocket *cmdSock);

    bool send_command(cstring command);

    ~Client();

    typedef enum{ ASCII, BIN} TransferType;

    TransferType _type;
    DataThread *dt_info;
    FileExplorer *fe;
    Thread<void, DataThread*, std::string, bool> *dt;
    Thread<void, Client*, std::string> *cmd;
    TcpSocket *cmdSocket;
    int pipe[2];

    sig_atomic_t active;
};

#endif //MASTER_SRC_CLIENT_H_
