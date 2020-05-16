

#include "DataThread.h"
#include "TcpSocket.h"
#include "AioTask.h"
#include "defines.h"
#include "request.h"
#include <unistd.h>
#include <fcntl.h>

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
    return port;
}

void DataThread::run(DataThread *datathread) { // it's will executed in new thread
    datathread->start("127,0,0,1");
    datathread->wait_commands();
}

DataThread::DataThread(TcpSocket *cmdSocket, FileExplorer *fe, int pipe) {
    this->cmdSocket = cmdSocket;
    this->pipe = pipe;
    this->fe = fe;
}

void DataThread::start(const std::string &ip) {
    // init passive connection
    TcpSocket *listening = new TcpSocket();
    uint16_t port = get_free_port(listening);
    cmdSocket->send(std::string("227 Entering Passive Mode (")
                        + ip + ','
                        + std::to_string(int(port / 256)) + ','
                        + std::to_string(port % 256) + ").\t\n");
    listening->listen(1);
    *dataSocket = listening->accept();

}

void DataThread::wait_commands() {
    const int buffer_size = 300;
    char buffer[buffer_size];
    if (read(pipe, buffer, buffer_size) == -1) {
        print_error("E: DataThread read from pipe failed");
        return;
    }
    request req = request(buffer);
    SWITCH(req.command().c_str()) { // It's my commands from cmd-thread
        CASE("SEND"):
            send(req.arg());
        break;
        CASE("RECV"):
            recv(req.arg());
        break;
        default:
            printf("%s\n", (std::string("unknown command ") + req.command()).c_str());
            break;
    }
}
void DataThread::send(const std::string &file_to) {
    // aio_read
    // send

}


void DataThread::recv(const std::string &to_file) {
    int out_fd = open(to_file.c_str(), O_WRONLY);
    if (out_fd == -1) {
        print_error("E: out_fd open failed");
        return;
    }
    int offset = 0;
    int readed = BUF_SIZE;
    AioTask task = AioTask(out_fd, false, buffer1, offset);
    bool first = true;
    do{

        if (readed == BUF_SIZE) {
            // blocking read from socket to buffer1
            printf("socket->recv(buffer1)\n");
            readed = dataSocket->recv_to_buffer(buffer1);
            if (readed == -1) {
                print_error("E: socket.recv(buffer1) failed");
                break;
            }
        }
        if (first){first = false;}
        else{
            printf("task2.wait\n");
            if (task.wait() == -1) {
                print_error("E: task2.wait() failed");
                break;
            }
            while(task.status() == EINPROGRESS){
                printf("active wait task2\n");
                sleep(1);
            }
        }
        // non-blocking write from buffer1 to file
        printf("task.write(buffer1)\n");
        task.set_offset(offset += readed);
        task.set_buffer(buffer1);
        if(!task.run()) {
            print_error("E: task.write(buffer1) failed");
            break;
        }
        if (readed == BUF_SIZE) {
            // blocking read from socket to buffer2
            printf("last socket->write(buffer2)\n");
            readed = dataSocket->recv_to_buffer(buffer2);
            if (readed == -1) {
                print_error("E: socket.recv(buffer1) failed");
                break;
            }
        }
        printf("task1.wait\n");
        if (task.wait() == -1) {
            print_error("E: task1.wait() failed");
            break;
        }
        while(task.status() == EINPROGRESS){
            printf("active wait task2\n");
            sleep(1);
        }
        if (readed != BUF_SIZE) {
            // non-blocking(aio) write from buffer2 to file
            printf("task2.write(buffer2)");
            task.set_buffer(buffer2);
            task.set_offset(offset += readed);
            if (!task.run()) {
                print_error("E: task2.write(buffer2) failed");
                break;
            }
        }
        else
            break;
    } while(true);
    close(out_fd);

}
