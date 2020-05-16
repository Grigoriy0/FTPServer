#if defined(__linux__) || defined(__unix__)
#define LIN_OS
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>

#endif

#define BUF_SIZE 256
#include "str_switch.h"

#include<stdio.h>
#include<string>

#ifndef PRINT_ERROR
#define PRINT_ERROR



#define print_error(msg) perror((std::string(msg)).c_str())

#endif // PRINT_ERROR


#ifndef cstring
#define cstring const std::string&
#endif // cstring
