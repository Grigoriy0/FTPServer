#include<string>
#include <cstring>
#include"defines.h"


#ifndef REQUEST_H
#define REQUEST_H

class Request {
private:
    std::string _command;
    std::string _arg;
    uchar index; // index of '\r' or ' '
private:
    void setCommand(cstring str) {
        _command = str.substr(0, index);
    }

    void setArg(cstring str) {
        if (index == 255 || str.c_str()[index] != ' ')
            _arg = "";
        else
            _arg = str.substr(index + 1, str.find_last_of(0x0D) - index - 1);
    }
public:
    Request(): index(0){}

    Request(cstring str) {
        index = str.find_first_of(" \r");
        setCommand(str);
        setArg(str);
    }

    Request &operator=(cstring str) {
        index = str.find_first_of(" \r");
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
