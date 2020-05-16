
#include <string>
#include "FTPServer.h"


//namespace global {
//    std::vector<Thread<void, TcpSocket*> *> cmd_threads;
//}
//
//struct Client {
//    DataThread *dt_info;
//    Thread<void, DataThread*, std::string> *dt;
//    TcpSocket *cmdSocket;
//    int pipe[2];
//
//    bool send_command(cstring command){
//        return write(pipe[1], command.c_str(), command.size()) != -1;
//    }
//
//};
//
//void createNewSession(TcpSocket* cmdClient){
//    global::cmd_threads.push_back(new Thread<void, TcpSocket*>{cmdThread, cmdClient});
//}

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

//    TcpSocket serverSocket = TcpSocket();
//
//    unsigned short port = std::atoi(argv[1]);
//    if (!serverSocket.bind(port)){
//        exit(-1);
//    }
//
//    if(!serverSocket.listen(1000)){
//        exit(-1);
//    }
//    printf("Server listening to port %d\n", port);
//
//    TcpSocket newClient(serverSocket.accept());
//    printf("Connection accepted\n");
//    createNewSession(&newClient);
//    serverSocket.shutdown();
//    serverSocket.close();
//
//    global::cmd_threads[0]->join();

	return 0;
}
