#include"aiotask.h"
#include"tcpsocket.h"
#include"mythread.h"

#define ERROR_SIGNAL SIGUSR2
#define COMMAND_SIGNAL SIGUSR2

struct Error{
    enum {INIT_HANDLERS_ERROR, AIO_ERROR}
    ErrorType;
    enum {SEND, RECV}
    TaskType;
    const std::string *msg;
};


inline uint16_t init_passive_mode_and_get_port(TcpSocket *dataChanel){
    uint16_t port;
    do{
        port = rand() % (uint16_t(-1) + 1024) - 1024;
    } while(!dataChanel->bind(port));
    return port;
}


void data_thread(TcpSocket *dtSk, Error *errorStash, pthread_t cmdThread){
    if (errorStash == nullptr)
        errorStash = new Error();
    if(AioTask::init_handlers() == false){
        errorStash->ErrorType = Error::INIT_HANDLERS_ERROR;
        pthread_kill(cmdThread, ERROR_SIGNAL);
    }

}

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
    Thread<void, TcpSocket*, Error*, pthread_t> th{data_thread,
            clientDtSk, nullptr, pthread_self()};
}


void new_command_handler(int sig, siginfo_t *si, void *ucontext){
    request cmd = *(std::string*)si->si_value.sival_ptr;
    if (cmd.command() == "SEND"){

    }
}

