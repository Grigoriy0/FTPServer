#include<stdio.h>
#include<errno.h>
#include<string>

#include"defines.h"
extern int connections;


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
