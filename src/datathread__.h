#include"AioTask.h"
#include"TcpSocket.h"
#include"mythread.h"
#include"fcntl.h"



void data_thread(TcpSocket *dtSk, pthread_t cmdThread, int *fd){
    //close(fd[1]);
    if(AioTask::init_handlers() == false){
        print_error("E: init_handlers");
    }
    char buffer1[BUF_SIZE];
    char buffer2[BUF_SIZE];
    printf("read(fd[0])\n");
    if (read(fd[0], buffer1, BUF_SIZE) == -1){
        print_error("E: read(fd[0]) failed");
    }
    printf("read done\n");
    std::string in = buffer1;
    std::string out = in.substr(in.find(' ') + 1, in.find_last_of(' ') - in.find(' ') - 1);
    in = in.substr(0, in.find(' '));
    printf("get command \"%s\" \"%s\"\n", in.c_str(), out.c_str());
    
    int in_fd = open(in.c_str(), O_RDONLY);
    if (in_fd == -1){
        print_error("E: in_fd open failed");
        return;
    }
    int out_fd = open(out.c_str(), O_WRONLY);
    if (out_fd == -1){
        print_error("E: out_fd open failed");
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
                print_error("E: unknown error");
                break;
            }
        }
        if (readed == 0) // read to buffer1
            break;
        if (readed == -1){
            print_error("E: read(in_fdm bf2) failed");
            break;
        }
        task.run();
        // blocking read            from file      to buffer2
        printf("read(buffer2)\n");
        readed = read(in_fd, buffer2, BUF_SIZE);
        // waiting aio write
        printf("task1.wait()\n");
        if (task.wait() == -1){
            print_error("E: task1.wait() failed");
            break;
        }
        while(task.status() == EINPROGRESS){
            printf("active wait task2\n");
            usleep(100000);
        }
        if (task.bytes() == -1){
            printf("bytes %d error %d\n", task.bytes(), task.status());
            print_error("E: aio_write(buffer1) failed");
            break;
        }
        if (readed == -1){
            print_error("E: read(in_fd, bf2) failed");
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

