#include<string>
#include <cstring>
#include"defines.h"


#ifndef REQUEST_H
#define REQUEST_H

class request {
private:
    std::string _command;
    std::string _arg;
    uchar index; // index of '\r' or ' '
private:
    void setCommand(cstring str) {
        _command = str.substr(0, index);
    }

    void setArg(cstring str) {
        if (str.c_str()[index] != ' ')
            _arg = "";
        else
            _arg = str.substr(index + 1, str.find_last_of(0x0D) - index - 1);
    }
public:
    request(): index(0){}

    request(cstring str) {
        index = str.find_first_of(" \r");
        setCommand(str);
        setArg(str);
    }

    request &operator=(cstring str) {
        index = str.find_first_of(" \r");
        printf("index = %d\n", (int)index);
        setCommand(str);
        setArg(str);
        return *this;
    }

    std::string command()const {
        return _command;
    }

    std::string arg()const {
        return _arg;
    }
};

#endif // REQUEST_H
