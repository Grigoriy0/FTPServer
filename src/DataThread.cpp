

#include "DataThread.h"
#include "TcpSocket.h"
#include "AioTask.h"
#include "defines.h"
#include "Request.h"
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>

static sig_atomic_t aio_abort = 0;


static void handler_abort(int sig) {// SIGABRT
    aio_abort = 1;
}

char buffer1[BUF_SIZE];
char buffer2[BUF_SIZE];


inline uint16_t get_free_port(TcpSocket *sk) {
    uint16_t port;
    do{
        port = rand() % (uint16_t(-1) + 1024) - 1024;
    } while(!sk->bind(port));
    printf("Selected %d port\n", (int)port);
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
    TcpSocket *listening = new TcpSocket();
    port = get_free_port(listening);
    printf("Port : %d\n", port);
    std::string reply = std::string("227 Entering Passive Mode (")
        + ip + ','
        + std::to_string(int(port / 256)) + ','
        + std::to_string(port % 256) + ").\r\n";
    printf("< %s", reply.c_str());
    cmdSocket->send(reply, 0);
    listening->listen(1);
    dataSocket = new TcpSocket(listening->accept());

}


void DataThread::start_active(std::string address) {
    address = address.substr(1, address.size() - 2);
    int ip1, ip2, ip3, ip4, port1, port2;
    sscanf(address.c_str(), "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &port1, &port2);
    uint16_t port = port1 * 256 + port2;
    address = std::to_string(ip1) +"."+std::to_string(ip2)+"."+std::to_string(ip3)+"."+std::to_string(ip4);
    TcpSocket *dtSock = new TcpSocket();
    if (!dtSock->connect(address, port)){
        print_error(std::string("E: dtSock.connect(") + address + ":" + std::to_string(port) + ")\r\n");
    }
    wait_commands();
}


void DataThread::wait_commands() {
    const int buffer_size = 300;
    char buffer[buffer_size];
    if (read(pipe[0], buffer, buffer_size) == -1) {
        print_error("E: DataThread read from pipe failed");
        return;
    }
    Request req = Request(buffer);
    SWITCH(req.command().c_str()) { // It's my commands from cmd-thread
        CASE("LIST"):
            AioTask::init_handlers();
            list(req.arg());
        break;
        CASE("SEND"):
            AioTask::init_handlers();
            send(req.arg());
        break;
        CASE("RECV"):
            AioTask::init_handlers();
            recv(fe->root_dir() + fe->pwd() + req.arg());
        break;
        default:
            printf("%s\n", (std::string("unknown command ") + req.command()).c_str());
            break;
    }
    active = false;
}

void DataThread::send(const std::string &file_to) {
    // aio_read
    // send

}


void DataThread::list(const std::string &dir) {
    std::vector<std::string> result = fe->ls(dir);
    for (auto &file: result)
        dataSocket->send(file);
    cmdSocket->send("250 Requested file action okay, completed.\r\n");
}

void DataThread::recv(const std::string &to_file) {
    printf("opening file %s\n", to_file.c_str());
    int out_fd = open(to_file.c_str(), O_WRONLY | O_CREAT);
    if (out_fd == -1) {
        print_error("E: out_fd open failed");
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
                break;
            }
        } else
            break;
        if (first){
            first = false;
        }
        else{
            printf("task2.wait\n");
            while ((res = task.wait()) == -1 && errno == EAGAIN);
            if (res == -1) {
                print_error("E: task2.wait() failed");
                break;
            }
            if (task.bytes() == 0) {
                break;
            }
            buffer2[task.bytes()] = 0;
        }
        if (task.bytes() != BUF_SIZE)
            break;
        // non-blocking write from buffer1 to file
        printf("readed = %d\nwrited = %d\n", readed, task.bytes());
        printf("task.write(buffer1)\n");
        task.set_offset(offset += readed);
        task.set_buffer(buffer1);
        if(!task.run(readed)) {
            print_error("E: task.write(buffer1) failed");
            break;
        }
        if (readed != 0) {
            // blocking read from socket to buffer2
            printf("last socket->write(buffer2)\n");
            readed = dataSocket->recv_to_buffer(buffer2, task.bytes());
            if (readed == -1) {
                print_error("E: socket.recv(buffer1) failed");
                break;
            }
            if (readed == 0)
                break;
        }
        printf("task1.wait\n");
        while ((res = task.wait()) == -1 && errno == EAGAIN);
        if (res == -1) {
            print_error("E: task1.wait() failed");
            break;
        }
        if (task.bytes() == 0) {
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
                break;
            }
        }
        else
            break;
    } while(true);
    close(out_fd);
    printf("Done\n");
}
