#include"aiotask.h"
#include <aio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>

#define bufSize 256
#define IO_SIGNAL SIGUSR1

bool AioTask::init_handlers(){
    struct sigaction sa{};
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = _quit_h;
    if (sigaction(SIGQUIT, &sa, nullptr) == -1){
        perror("sigaction _quit_h\n");
        return false;
    }
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sa.sa_sigaction = _aio_done_h;
    if (sigaction(IO_SIGNAL, &sa, nullptr) == -1){
        perror("sigaction _aio_done_h\n");
        return false;
    }
    return true;
}

AioTask::AioTask(int fd, bool read, char *buffer, int offset){
    _fd = fd;
    _buffer = buffer;
    _type = read ? SEND : RECV;
    _cb = (aiocb*)calloc(1, sizeof(aiocb));
    _cb->aio_fildes = _fd;
    _cb->aio_buf = _buffer;
    _cb->aio_reqprio = 0;
    _cb->aio_offset = offset;
    _cb->aio_nbytes = bufSize;
    _cb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    _cb->aio_sigevent.sigev_signo = IO_SIGNAL;
    _cb->aio_sigevent.sigev_value.sival_ptr = this;
}

void AioTask::set_buffer(char *buffer){
    _buffer = buffer;
}

void AioTask::run(){
    if (_type == SEND)
        aio_read(_cb);
    else
        aio_write(_cb);
}

int AioTask::bytes(){
    return _bytes;
}

void AioTask::set_offset(int offset){
    _cb->aio_offset = offset;
}

int AioTask::get_offset(){
    return _cb->aio_offset;
}

void AioTask::cancel(){
    aio_cancel(_fd, _cb);
}

int AioTask::wait(int milliseconds){
    timespec t{};
    t.tv_nsec = milliseconds * 1000;
    return aio_suspend(&_cb, 1, &t);
}

// private static handlers:

void AioTask::_aio_done_h(int sig, siginfo_t *si, void *ucontext){
    AioTask* task = (AioTask*)si->si_value.sival_ptr;
    task->_bytes = aio_return(task->_cb);
    printf("I/O completion signal received butes %d\n", _bytes);
}

void AioTask::_quit_h(int sig){
    gotSIGQUIT = 1;
}

