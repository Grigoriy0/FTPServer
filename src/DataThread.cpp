

#include "DataThread.h"
#include "TcpSocket.h"
#include "AioTask.h"
#include "defines.h"
#include "Request.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#include <random>
#include <algorithm>

static sig_atomic_t aio_abort = 0;


static void handler_abort(int sig) {// SIGABRT
    aio_abort = 1;
}


inline uint16_t bind_free_port(TcpSocket *sk, uint16_t from_range = 1024, uint16_t to_range = 3000) {
    uint16_t port;
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(from_range, to_range);
    do{
        port = distribution(generator);
    } while(!sk->bind(port));
    return port;
}

void DataThread::run(DataThread *datathread, std::string data, bool activeMode) { // it's will executed in new thread
    printf("DataThread running in %s mode\n", activeMode?"active":"passive");
    datathread->active = true;
    if (activeMode)
        datathread->start_active(data);
    else
        datathread->start_passive(data);
    printf("Wait for commands from cmd thread using pipe\n");
    datathread->wait_commands();
    datathread->active = false;
}

DataThread::DataThread(TcpSocket *cmdSocket, cstring root_dir, int *pipe) {
    this->cmdSocket = cmdSocket;
    this->pipe = pipe;
    this->fe = new FileExplorer(root_dir);
    this->active = false;
}

void DataThread::start_passive(std::string ip) {
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
        return;
    }
    printf("waiting for client connection to data port\n");
    reply = std::string("227 Entering Passive Mode (")
        + ip + ','
        + std::to_string(int(port / 256)) + ','
        + std::to_string(port % 256) + ").\r\n";
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);
    dataSocket = new TcpSocket(listening->accept());

}


void DataThread::start_active(std::string address) { //129.168.0.1
    address = address.substr(1, address.size() - 2);
    int ip1, ip2, ip3, ip4, port1, port2;
    sscanf(address.c_str(), "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &port1, &port2);
    uint16_t port = port1 * 256 + port2;
    address = std::to_string(ip1) + "." + std::to_string(ip2) + "." + std::to_string(ip3) + "." + std::to_string(ip4);
    TcpSocket *dtSock = new TcpSocket();
    if (!dtSock->connect(address, port)) {
        print_error(std::string("E: dtSock.connect(") + address + ":" + std::to_string(port) + ")\r\n");
        std::string reply = "500 Error connection to your host\r\n";
        printf("< %s", reply.c_str());
        cmdSocket->send(reply);
        return;
    }
    std::string reply = "200 Command OK\r\n";
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);
    wait_commands();
}


void DataThread::wait_commands() {
    const int buffer_size = 300;
    char buffer[buffer_size];
    if (read(pipe[0], buffer, buffer_size) == -1) {
        print_error("E: DataThread read from pipe failed");
        std::string reply = "500 Error on the server while reading from pipe\r\n";
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
        default:
            printf("%s\n", (std::string("unknown command ") + req.command()).c_str());
            break;
    }
    dataSocket->shutdown();
    dataSocket->close();
    active = false;
}

void DataThread::send(const std::string &file_from) {
    std::string reply = "226 Data transfered\r\n";
    printf("opening file %s\n", file_from.c_str());
    int fd;
    if ((fd = open(file_from.c_str(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP)) == -1) {
        print_error("open failed");
        reply = "500 Error file not found\r\n";
        printf("< %s", reply.c_str());
        cmdSocket->send(reply);
        return;
    }
    off_t offset;
    const ssize_t count = BUF_SIZE;
    int writed = 0;
    int error_count = 0;
    do {
        writed = ::sendfile(dataSocket->getFD(), fd, &offset, count); // #include<sys/sendfile.h>
        if (writed == -1) {
            print_error("sendfile failed offset = " + std::to_string(offset));
            if (++error_count == 3) {
                print_error("3 times error");
                reply = "500 Error on the server while io operations\r\n";
                printf("< %s", reply.c_str());
                cmdSocket->send(reply);
                return;
            }
        }
        else {
            error_count = 0;
            offset += BUF_SIZE;
        }
    } while(writed == BUF_SIZE);
    printf("Done\n");
    close(fd);
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);
}


void DataThread::list(const std::string &dir) {
    std::string reply = "125 Transfer starting\r\n";
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);

    std::vector<std::string> file_list = fe->ls(dir);

    std::string result;
    for (auto &file: file_list)
        result += file + '\n';
    dataSocket->send(result);
    cmdSocket->send("226 Requested file action okay, completed.\r\n");
}

void DataThread::recv(const std::string &to_file) {
    char buffer1[BUF_SIZE];
    char buffer2[BUF_SIZE];

    AioTask::init_handlers();
    printf("opening file %s\n", (to_file).c_str());
    std::string reply;
    int out_fd = open(to_file.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
    if (out_fd == -1) {
        print_error("E: out_fd open failed");
        reply = "500 Error opening file\r\n";
        printf("< %s", reply.c_str());
        cmdSocket->send(reply);
        return;
    }
    int offset = 0;
    int readed = BUF_SIZE;
    AioTask task = AioTask(out_fd, false, buffer1, offset);
    bool first = true;
    do{
        int res;

        if (readed == BUF_SIZE) {
            // blocking read from socket to buffer1
            printf("socket->recv(buffer1)\n");
            readed = dataSocket->recv_to_buffer(buffer1);
            if (readed == -1) {
                print_error("E: socket.recv(buffer1) failed");
                reply = "500 Error on the server while io operations\r\n";
                break;
            }
        } else
            break;
        if (first){
            first = false;
        }
        else {
            printf("task2.wait\n");
            while ((res = task.wait()) == -1 && errno == EAGAIN);
            if (res == -1) {
                print_error("E: task2.wait() failed");
                reply = "500 Error on the server while io operations\r\n";
                break;
            }
            if (task.bytes() == 0) {
                break;
            }
            buffer2[task.bytes()] = 0;
        }
        if (task.bytes() != BUF_SIZE) {
            reply = "226 Data transfered\r\n";
            break;
        }
        // non-blocking write from buffer1 to file
        printf("readed = %d\nwrited = %d\n", readed, task.bytes());
        printf("task.write(buffer1)\n");
        task.set_offset(offset += readed);
        task.set_buffer(buffer1);
        if(!task.run(readed)) {
            print_error("E: task.write(buffer1) failed");
            reply = "500 Error on the server while io operations\r\n";
            break;
        }
        if (readed != 0) {
            // blocking read from socket to buffer2
            printf("last socket->write(buffer2)\n");
            readed = dataSocket->recv_to_buffer(buffer2, task.bytes());
            if (readed == -1) {
                print_error("E: socket.recv(buffer1) failed");
                reply = "500 Error on the server while io operations\r\n";
                break;
            }
            if (readed == 0) {
                reply = "226 Data transfered\r\n";
                break;
            }
        }
        printf("task1.wait\n");
        while ((res = task.wait()) == -1 && errno == EAGAIN);
        if (res == -1) {
            print_error("E: task1.wait() failed");
            reply = "500 Error on the server while io operations\r\n";
            break;
        }
        if (task.bytes() == 0) {
            reply = "226 Data transfered\r\n";
            break;
        }
        buffer1[task.bytes()] = 0;
        if (readed == task.bytes()) {
            // non-blocking(aio) write from buffer2 to file
            printf("task2.write(buffer2)");
            task.set_buffer(buffer2);
            task.set_offset(offset += readed);
            if (!task.run(readed)) {
                print_error("E: task2.write(buffer2) failed");
                reply = "500 Error on the server while io operations\r\n";
                break;
            }
        }
        else {
            reply = "226 Data transfered\r\n";
            break;
        }
    } while(true);
    close(out_fd);
    printf("< %s", reply.c_str());
    cmdSocket->send(reply);
}
