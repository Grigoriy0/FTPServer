

#include "DataThread.h"
#include "TcpSocket.h"
#include "defines.h"
#include "Request.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#include <random>
#include <algorithm>
#include <sys/stat.h>



inline uint16_t bind_free_port(TcpSocket *sk, uint16_t from_range = 2000, uint16_t to_range = 3000) {
    uint16_t port;
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(from_range, to_range);
    do{
        port = distribution(generator);
    } while(!sk->bind(port));
    return port;
}

void DataThread::run(DataThread *datathread, std::string data, bool activeMode) { // it's will executed in new thread
    datathread->active = true;
    bool yes;
    if (activeMode)
        yes = datathread->start_active(data);
    else
        yes = datathread->start_passive(data);
    if (!yes) {
        datathread->active = false;
        return;
    }
    datathread->wait_commands();
    datathread->active = false;
}

DataThread::DataThread(TcpSocket *cmdSocket, FileExplorer* fe, int *pipe) {
    this->cmdSocket = cmdSocket;
    this->pipe = pipe;
    this->fe = fe;
    this->active = false;
}

bool DataThread::start_passive(std::string ip) {
    std::replace(ip.begin(), ip.end(), '.', ',');
    // init passive connection
    std::string reply;
    TcpSocket *listening = new TcpSocket();
    port = bind_free_port(listening);
    printf("Port : %d\n", port);
    if (!listening->listen(1)) {
        print_error("E: Cannot listening port");
        reply = "500 Error on the server\r\n";
        printf("< %s", reply.c_str());
        cmdSocket->send(reply);
        return false;
    }
    reply = std::string("227 Entering Passive Mode (")
        + ip + ','
        + std::to_string(int(port / 256)) + ','
        + std::to_string(port % 256) + ").\r\n";
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);
    dataSocket = new TcpSocket(listening->accept());
    return true;
}


bool DataThread::start_active(std::string address) { //129,168,0,106,port1,port2
    int ip1, ip2, ip3, ip4, port1, port2;
    std::replace(address.begin(), address.end(), ',', ' ');
    sscanf(address.c_str(), "%d%d%d%d%d%d", &ip1, &ip2, &ip3, &ip4, &port1, &port2);

    uint16_t port = port1 * 256 + port2;
    address = std::to_string(ip1) + "." + std::to_string(ip2) + "." + std::to_string(ip3) + "." + std::to_string(ip4);

    dataSocket = new TcpSocket();
    printf("Connecting to %s:%hu\n", address.c_str(), port);
    if (!dataSocket->connect(address, port)) {
        print_error(std::string("E: dtSock.connect(") + address + ":" + std::to_string(port) + ")\r\n");
        std::string reply = "500 Error connection to your host\r\n";
        printf("< %s", reply.c_str());
        cmdSocket->send(reply);
        return false;
    }
    std::string reply = "200 Command OK\r\n";
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);
    wait_commands();
    return true;
}


void DataThread::wait_commands() {
    const int buffer_size = 300;
    char buffer[buffer_size];
    if (read(pipe[0], buffer, buffer_size) == -1) {
        print_error("E: DataThread read from _pipe failed");
        std::string reply = "500 Error on the server while IP operations\r\n";
        printf("< %s", reply.c_str());
        cmdSocket->send(reply);
        return;
    }
    Request req = Request(buffer);
    SWITCH(req.command().c_str()) { // It's my commands from cmd-thread
        CASE("LIST"):
            list(req.arg());
        break;
        CASE("SEND"):
            send(fe->root_dir() + fe->pwd() + req.arg());
        break;
        CASE("RECV"):
            recv(fe->root_dir() + fe->pwd() + req.arg());
        break;
        default:break;
    }
    dataSocket->shutdown();
    dataSocket->close();
    active = false;
}

void DataThread::send(const std::string &file_from) {
    std::string reply = "226 Data transferred\r\n";
    int fd;
    if ((fd = open(file_from.c_str(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP)) == -1) {
        print_error("open failed");
        reply = "500 Error file not found\r\n";
        printf("< %s", reply.c_str());
        cmdSocket->send(reply);
        return;
    }
    off_t offset = 0;
    struct stat file_stat;
    fstat(fd, &file_stat);
    ssize_t size = file_stat.st_size;
    ssize_t sent = ::sendfile(dataSocket->getFD(), fd, &offset, size);
    printf("sent = %zd Bytes\n", sent);
    close(fd);
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);
}


void DataThread::list(const std::string &dir) {
    std::vector<std::string> file_list = fe->ls(dir);
    if (!file_list.empty() && file_list[0] == "~~~") {
        file_list[0] = "<Error directory not found>";
    }
    std::string result;
    for (auto &file: file_list)
        result += file + "\r\n";
    dataSocket->send(result);
    const std::string reply = "226 It's all list of files\r\n";
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);
}

void DataThread::recv(const std::string &to_file) {
    char buffer[BUF_SIZE + 1];
    std::string reply;
    int out_fd = open(to_file.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
    if (out_fd == -1) {
        print_error("E: out_fd open failed");
        reply = "500 Error opening file\r\n";
        printf("< %s", reply.c_str());
        cmdSocket->send(reply);
        return;
    }
    int readed = 0;
    ssize_t all = 0;
    int wrote = 0;
    do {
        readed = dataSocket->recv_to_buffer(buffer);
        if (readed == -1) {
            break;
        }

        buffer[readed] = 0;

        if ((wrote = write(out_fd, buffer, readed)) == -1) {
            print_error("write to file failed");
            reply = "500 Error on the server while io operations\r\n";
            break;
        }
        if (readed == 0) {
            reply = "226 Data transferred\r\n";
            break;
        }

        all += readed;

    } while(true);
    printf("received = %zd Bytes\n", all);
    close(out_fd);
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);
}
