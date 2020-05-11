#include"aiotask.h"
#include"tcpsocket.h"
#include"mythread.h"
#include"fcntl.h"

#define ERROR_SIGNAL SIGUSR2

inline uint16_t init_passive_mode_and_get_port(TcpSocket *dataChanel){
    uint16_t port;
    do{
        port = rand() % (uint16_t(-1) + 1024) - 1024;
    } while(!dataChanel->bind(port));
    return port;
}

void data_thread(TcpSocket *dtSk, pthread_t cmdThread, int*fd);

void set_up_passive(TcpSocket *cmdChanel){
    TcpSocket* datChanel = new TcpSocket();
    uint16_t port = init_passive_mode_and_get_port(datChanel);
    cmdChanel->send(std::string("227 Entering Passive Mode (")
            + std::to_string(127) + ','
            + std::to_string(0)   + ','
            + std::to_string(0)   + ','
            + std::to_string(1)   + ','
            + std::to_string(int(port / 256)) + ','
            + std::to_string(port % 256));
    std::to_string(datChanel->listen(1));
    TcpSocket *clientDtSk = new TcpSocket();
    *clientDtSk = datChanel->accept();
    int fd[2];
    pipe(fd);
    Thread<void, TcpSocket*, pthread_t, int*> th{data_thread, clientDtSk, pthread_self(), fd};
    close(fd[0]);
}


void data_thread(TcpSocket *dtSk, pthread_t cmdThread, int *fd){
    //close(fd[1]);
    if(AioTask::init_handlers() == false){
        print_error("init_handlers");
    }
    char buffer1[BUF_SIZE];
    char buffer2[BUF_SIZE];
    printf("read(fd[0])\n");
    if (read(fd[0], buffer1, BUF_SIZE) == -1){
        print_error("read(fd[0]) failed");
    }
    printf("read done\n");
    std::string in = buffer1;
    std::string out = in.substr(in.find(' ') + 1, in.find_last_of(' ') - in.find(' ') - 1);
    in = in.substr(0, in.find(' '));
    printf("get command \"%s\" \"%s\"\n", in.c_str(), out.c_str());
    
    int in_fd = open(in.c_str(), O_RDONLY);
    if (in_fd == -1){
        print_error("in_fd open failed");
        return;
    }
    int out_fd = open(out.c_str(), O_WRONLY);
    if (out_fd == -1){
        print_error("out_fd open failed");
        return;
    }
    int offset = 0;
    int readed = 0;
    AioTask task = AioTask(out_fd, false, buffer1, offset);
    bool first = true;
    do{
        // blocking read            from file      to buffer1
        printf("read(in_fd)\n");
        readed = read(in_fd, buffer1, BUF_SIZE);
        // non-blocking(aio) write  from buffer1   to socket
        printf("task.run(buffer1)\n");
        if (first){
            first = false;
        }else{
            int s = task.status();
            while (s == EINPROGRESS){
                printf("active wait task2\n");
                usleep(100000);
                s = task.status();
            }
            if (s == 0){
                printf("task success ends\n");
                task.set_buffer(buffer1);
                task.set_offset(offset += BUF_SIZE);
            }else {
                printf("task.status returns %d\n", s);
                print_error("unknown error");
                break;
            }
        }
        if (readed == 0) // read to buffer1
            break;
        if (readed == -1){
            print_error("read(in_fdm bf2) failed");
            break;
        }
        task.run();
        // blocking read            from file      to buffer2
        printf("read(buffer2)\n");
        readed = read(in_fd, buffer2, BUF_SIZE);
        // waiting aio write
        printf("task1.wait()\n");
        if (task.wait() == -1){
            print_error("task1.wait() failed");
            break;
        }
        while(task.status() == EINPROGRESS){
            printf("active wait task2\n");
            usleep(100000);
        }
        if (task.bytes() == -1){
            printf("bytes %d error %d\n", task.bytes(), task.status());
            print_error("aio_write(buffer1) failed");
            break;
        }
        if (readed == -1){
            print_error("read(in_fd, bf2) failed");
            break;
        }
        if (readed == 0) // readed to buffer2
            break;
        task.set_buffer(buffer2);
        task.set_offset(offset += BUF_SIZE);
        task.run();
        // non-blocking(aio) write  from buffer2   to socket
    } while(true);
    close(in_fd);
    close(out_fd);
}

