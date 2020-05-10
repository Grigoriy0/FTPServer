#include<string>
#include"defines.h"
#include"str_switch.h"


class request {
private:
    std::string _command;
    std::string _arg;
    uchar index; // index of '\t' or ' '
private:
    void setCommand(cstring str) {
        _command = str.substr(0, index);
    }

    void setArg(cstring str) {
        if (str.c_str()[index] != ' ')
            _command = "";
        else
            _command = str.substr(index + 1, str.find(0x0D) - index - 1);
    }
public:
    request(): index(0){}

    request(cstring str) {
        index = str.find(" \t");
        setCommand(str);
        setArg(str);
    }

    request operator=(cstring str) {
        index = str.find(" \t");
        setCommand(str);
        setArg(str);
    }

    std::string command()const {
        return _command;
    }

    std::string arg()const {
        return _arg;
    }
};
