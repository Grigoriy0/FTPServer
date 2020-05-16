

#include <cstring>
#include "TcpSocket.h"
#include "FileExplorer.h"
#include "DataThread.h"
#include "FTPServer.h"
#include "MySqlClient.h"
#include "Request.h"
#include <sys/socket.h>
#include <arpa/inet.h>

FTPServer::FTPServer(cstring root, uint16_t port, int max_conn) :
port(port), root_dir(root), max_connections(max_conn) {
    listening = new TcpSocket();
    if(!listening->bind(port)){
        printf("E: cannot bind port to listening socket\nExit\n");
        listening->close();
        exit(2);
    }
    listening->listen(max_connections);
    connections = 0;
    get_my_ip();
    printf("and the port %d\n", port);
    do {
        connection_handler( new TcpSocket(listening->accept()) );
    } while(connections < max_connections);
}

MySqlClient::DBUser auth(TcpSocket *client) {
    Request request;
    std::string tmp;
    std::string reply;

    request = tmp = client->recv();
    printf("> %s\n", tmp.c_str());

    while(request.command() != "USER") {
        reply = "130 Sign in first\r\n";
        printf("< %s", reply.c_str());
        client->send(reply);

        request = tmp = client->recv();
        printf("> %s\n", tmp.c_str());
    }


    MySqlClient::DBUser user;
    user.uname = request.arg();

    MySqlClient mysql("FTPServer");
    if (!mysql.connect("root", "2349914")) {
        print_error(mysql.last_error().c_str());
        reply = "530 Sorry, error on the database server\r\n";
        printf("< %s", reply.c_str());
        client->send(reply);
        mysql.disconnect();
        client->close();
        return {};
    }
    if (user.uname != "anonymous")
        reply = "331 Password required\r\n";
    else
        reply = "230 OK\r\n";
    printf("< %s", reply.c_str());
    client->send(reply);


    if (user.uname != "anonymous") {
        request = tmp = client->recv();
        printf("> %s\n", tmp.c_str());
        user.perm.id = mysql.auth(user.uname, request.arg());

        if (user.perm.id > 0) {
            reply = "230 Log in as user " + user.uname + "\r\n";
            printf("< %s", reply.c_str());
            client->send(reply);

        }
        else {
            reply = "530 Unknown user " + user.uname + "\r\n";
            printf("> %s", reply.c_str());
            client->send(reply);
            mysql.disconnect();
            client->close();
            return {};
        }
    }
    user = mysql.getUserInfo(user.perm.id);
    mysql.disconnect();
    return user;
}

void cmdThread(TcpSocket *client) {

    std::string reply;
    Request buffer;
    std::string tmp;
    FTPServer::Client client_info{};

    client_info.cmdSocket = client;

    reply = "220 Welcome to FTP server by Grigoriy!\r\n";
    printf("< %s", reply.c_str());
    client->send(reply);

    MySqlClient::DBUser user;
    user = auth(client);

    printf("Start communication\n");
    bool quit = false;

    std::string pwd = user.homedir;
    FileExplorer fe = FileExplorer(user.homedir);
    do {
        tmp = client->recv();
        if (tmp.empty()) {
            print_error("E: empty command from server");
            break;
        }
        buffer = tmp;
        printf("> \"%s\"\n", tmp.c_str());
        SWITCH(buffer.command().c_str()) {
            CASE("NOOP"):
                reply = "200 OK\r\n";
                break;
                CASE("SYST"):
                reply = "215 Unix\r\n";
                break;
                CASE("STAT"):
                reply = std::string("211 Logged in ") + user.uname + "\n" + "211 End of status\r\n";
                break;
                CASE("QUIT"):
                reply = "221 Goodbye\r\n";
                quit = true;
                break;
                CASE("HELP"):
                reply = "500 I don't have man for this command\r\n";
                break;
                CASE("PWD"):
                reply = "257 \"" + fe.pwd() + "\" is the current directory\r\n";
                break;
                CASE("MKD"):
            {
                if (fe.mkdir(buffer.arg()))
                    reply = "230 Created dir " + buffer.arg() + "\r\n";
                else
                    reply = "530 Error creating dir\r\n";
            }
                break;
                CASE("RMD"):
            {
                if (fe.rmdir(buffer.arg()))
                    reply = "230 Folder was removed\r\n";
                else
                    reply = "530 Error creating folder\r\n";
            }
                break;
                CASE("SIZE"):
            {
                uint64_t size = fe.size(buffer.arg());
                if (size != -1)
                    reply = "230 Size = " + std::to_string(size) + "\r\n";
                else
                    reply = "530 Error getting size of file " + buffer.arg() + "\r\n";
            }
                break;
                CASE("DELE"):
                if (fe.rm(buffer.arg()))
                    reply = "230 File " + buffer.arg() + "deleted\r\n";
                else
                    reply = "530 Error deleting file\r\n";
                break;
                CASE("PASV"):
            {
                int p[2];
                if (pipe(p) == -1) {
                    print_error("E: pipe failed");
                    reply = "550 Error on the server\r\n";
                    break;
                }
                else reply = "";
                client_info.dt_info = new DataThread(client, user.homedir, p);
                memcpy(client_info.pipe, p, 2 * sizeof(int));
                // start thread
                client_info.dt = new Thread<void, DataThread*, std::string>{DataThread::run, client_info.dt_info, "25,9,120,152"};
            }
                break;
            default:
//                quit = true;
                reply = "500 Unknown command\r\n";
                break;
        }

        if (!reply.empty()) {
            printf("< %s", reply.c_str());
            if (client->send(reply) == 0){
                print_error("E: send failed");
                quit = true;
            }
        }

    } while(!quit);

    printf("Close socket\n");
    client->close();
    exit(0);
}


void FTPServer::connection_handler(TcpSocket *sock) {
    printf("Connection accepted\n");
    Client *client = new Client;
    client->cmdSocket = sock;
    client->cmd = new Thread<void, TcpSocket*>{cmdThread, sock};
    clients.push_back(client);
}

void FTPServer::get_my_ip() {

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    const char* kGoogleDnsIp = "8.8.8.8";
    uint16_t kDnsPort = 53;
    struct sockaddr_in serv{};
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons(kDnsPort);

    int err = connect(sock, (const sockaddr*) &serv, sizeof(serv));

    sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (sockaddr*) &name, &namelen);
    char buf[INET_ADDRSTRLEN];
    ip = inet_ntop(AF_INET, &name.sin_addr, buf, sizeof(buf));
    printf("IPv4 %s ", ip.c_str());
    close(sock);
}