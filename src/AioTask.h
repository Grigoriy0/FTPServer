#include"TcpSocket.h"
#include"Request.h"
#include"defines.h"
#include<aio.h>
#include<signal.h>

#ifndef AIO_TASK_H
#define AIO_TASK_H

#define IO_SIGNAL SIGUSR1

class AioTask{
public:
    static bool init_handlers();

    AioTask(int fd, bool read, char *buffer, int offset);

    void cancel();

    int bytes();

    int wait(int milliseconds = 5000);

    bool run();

    int status();

    void set_buffer(char* buffer);

    void set_offset(int offset);

    int get_offset();
private:
    enum {SEND, RECV}
        _type;
    char * _buffer;
    int _fd;
    bool _done;
    aiocb *_cb;
    int _bytes;
    int _offset;
    static void _aio_done_h(int sig, siginfo_t *si, void *ucontext);
    static void _quit_h(int sig);
    static volatile sig_atomic_t gotSIGQUIT;
};


#endif // AIO_TASK_H
