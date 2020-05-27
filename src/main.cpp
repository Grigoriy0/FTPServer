
#include <string>
#include <arpa/inet.h>
#include "FTPServer.h"



int main(int argc, char *argv[]) {
    const char* usage = "Usage: ./Server [port] [address] [max_connections]";
    const char* env_requirements = "Please set required environment variables:\n"
                                   "FTP_DB_NAME - name of mysql database name\n"
                                   "FTP_DB_USER - username for connection\n"
                                   "FTP_DB_PASS - password for user (optional)";

    if (getenv("FTP_DB_NAME") == nullptr || getenv("FTP_DB_USER") == nullptr){
        printf("%s\n", env_requirements);
        return 1;
    }

    uint16_t port;
    int max_conn = 1000;
    std::string public_ip;
    srand(time(nullptr));


    if (argc < 2) {
        printf("%s\n", usage);
        return 1;
    }
    if (argc > 1) {
        if ((port = atoi(argv[1])) == 0) {
            port = 21;
        }
        if (argc > 2) {
            if (inet_pton(AF_INET, argv[2], malloc(15)) == 1)
                public_ip = argv[2];
            else{
                printf("%s\n", usage);
                return 1;
            }
            if (argc > 3 &&
                (max_conn = atoi(argv[2])) == 0) {
                max_conn = 1000;
            }
        }
    }
    else port = 21;

    std::string root_dir = "/ftpd/";



    FTPServer server(root_dir, port, public_ip, max_conn);

	return 0;
}