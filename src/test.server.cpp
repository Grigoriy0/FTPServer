
#include <string>
#include "FTPServer.h"



int main(int argc, char *argv[]) {
    const char* usage = "Usage: ./Server [port] [max_connections]";
    if (argc < 2) {
        printf("%s\n", usage);
        return 1;
    }
    uint16_t port;
    int max_conn = 1000;
    if (argc > 1) {
        if ((port = atoi(argv[1])) == 0) {
            port = 21;
        }
        if (argc > 2 &&
        (max_conn = atoi(argv[2])) == 0) {
            max_conn = 1000;
        }
    }
    else port = 21;

    std::string dir = "/run/";

    FTPServer server(dir, port, max_conn);

	return 0;
}