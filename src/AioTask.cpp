#include"AioTask.h"
#include <aio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>


bool AioTask::init_handlers(){
    struct sigaction sa{};
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = _quit_h;
    if (sigaction(SIGQUIT, &sa, nullptr) == -1){
        print_error("E: sigaction(SIGQUIT) failed");
        return false;
    }
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sa.sa_sigaction = _aio_done_h;
    if (sigaction(IO_SIGNAL, &sa, nullptr) == -1){
        print_error("E: sigaction(IO_SIGNAL) failed");
        return false;
    }
    return true;
}

AioTask::AioTask(int fd, bool read, char *buffer, int offset){
    _fd = fd;
    _buffer = buffer;
    _offset = offset;
    _type = read ? SEND : RECV;
    _cb = (aiocb*)calloc(1, sizeof(aiocb));
    _cb->aio_fildes = _fd;
    _cb->aio_buf = _buffer;
    _cb->aio_reqprio = 0;
    _cb->aio_offset = offset;
    _cb->aio_nbytes = BUF_SIZE;
    _cb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    _cb->aio_sigevent.sigev_signo = IO_SIGNAL;
    _cb->aio_sigevent.sigev_value.sival_ptr = this;
}

void AioTask::set_buffer(char *buffer){
    _buffer = buffer;
}

bool AioTask::run() {
    _cb->aio_offset = _offset;
    _cb->aio_buf = _buffer;
    if (_type == SEND) {
        if (aio_read(_cb) == -1) {
            print_error("E: aio_read failed");
            return false;
        }
    } else if (aio_write(_cb) == -1) {
        print_error("E: aio_write failed");
        return false;
    }
}

int AioTask::status(){
    return aio_error(_cb);
}


int AioTask::bytes(){
    return _bytes;
}

void AioTask::set_offset(int offset){
    _offset = offset;
}

int AioTask::get_offset(){
    return _cb->aio_offset;
}

void AioTask::cancel(){
    if(aio_cancel(_fd, _cb) == -1)
        print_error("E: aio_cancel failed");
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
    printf("I/O completion signal received bytes %d\n", task->_bytes);
}

void AioTask::_quit_h(int sig){
    gotSIGQUIT = 1;
}

volatile sig_atomic_t AioTask::gotSIGQUIT = 0;
