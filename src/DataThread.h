#ifndef DATA_THREAD_H
#define DATA_THREAD_H

#include <string>
#include "FileExplorer.h"
#include "defines.h"

class TcpSocket;

class DataThread {
public:
    explicit DataThread(TcpSocket *cmdSocket, FileExplorer *fe, int *pipe);

    static void run(DataThread *datathread, std::string data, bool activeMode);

    bool works(){ return active;}

private:
    bool start_passive(std::string ip);

    bool start_active(std::string address);

    void wait_commands();

    void send(const std::string& file_to);

    void recv(const std::string& to_file);

    void list(const std::string &dir);

    int *pipe;                    //read-only
    TcpSocket *dataSocket;
    TcpSocket *cmdSocket;
    FileExplorer *fe;
    uint16_t port;
    sig_atomic_t active;
};

#endif // DATA_THREAD_H