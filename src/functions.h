#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/param.h>
#include<errno.h>
#include<pthread.h>
#include<string>

#include"tcpsocket.h"
#include"DBServer.h"
static int connections = 0;

bool checkCommandSupporting(const std::string &command){
    std::string word = command.substr(0, command.find(' '));
    return word == "QUIT" ||
        word == "USER" ||
        word == "PASS" ||
        word == "SIZE" ||
        word == "LIST" ||
        word == "CDUP" ||
        word == "DELE" ||
        word == "RMD"  ||
        word == "MKD"  ||
        word == "PWD"  ||
        word == "SYST" ||
        word == "NOOP" ||
        word == "HELP";
}

void childExitedHandler(int signum, siginfo_t *info, void *ptr){
    switch(info->si_code){
        case CLD_EXITED:
            connections--;
            printf("Session %d has finished with code %d\n", info->si_pid, info->si_status);
            break;
        case CLD_KILLED:
            connections--;
            printf("Session %d was killed\n", info->si_pid);
            break;
        default:
            break;

    }
    printf("si_status %d\n", info->si_status);
}
