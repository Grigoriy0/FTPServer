#ifndef DATA_THREAD_H
#define DATA_THREAD_H

#include <string>
#include "FileExplorer.h"
#include "defines.h"

class TcpSocket;

class DataThread {
public:
    explicit DataThread(TcpSocket *cmdSocket, cstring root_dir, int *pipe);

    static void run(DataThread *datathread, std::string ip);

private:
    void start(const std::string &ip);

    void wait_commands();

    void send(const std::string& file_to);

    void recv(const std::string& to_file);

    int *pipe;                    //read-only
    TcpSocket *dataSocket;
    TcpSocket *cmdSocket;
    FileExplorer *fe;
    uint16_t port;
};

#endif // DATA_THREAD_H