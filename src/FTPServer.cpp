

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
    if(!listening->bind(port)) {
        printf("E: cannot bind port to listening socket\nExit\n");
        listening->close();
        exit(2);
    }
    listening->listen(max_connections);
    connections = 0;
    get_my_ip();
    printf("and the port %d\n", port);
    clients = std::vector<Client*>{};
    do {
        connection_handler( new TcpSocket(listening->accept()) );
        int i = 0;
        for (; i < clients.size(); ++i) {
            if (!clients[i]->active) {
                clients.erase(clients.begin() + i);
                --i;
                --connections;
            }
        }

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
        client->send(reply, 0);

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
        client->send(reply, 0);
        mysql.disconnect();
        client->shutdown();
        client->close();
        return {};
    }
    if (user.uname != "anonymous")
        reply = "331 Password required\r\n";
    else
        reply = "230 OK\r\n";
    printf("< %s", reply.c_str());
    client->send(reply, 0);


    if (user.uname != "anonymous") {
        request = tmp = client->recv();
        printf("> %s\n", tmp.c_str());
        user.perm.id = mysql.auth(user.uname, request.arg());

        if (user.perm.id > 0) {
            reply = "230 Log in as user " + user.uname + "\r\n";
            printf("< %s", reply.c_str());
            client->send(reply, 0);

        }
        else {
            reply = "530 Unknown user " + user.uname + "\r\n";
            printf("> %s", reply.c_str());
            client->send(reply, 0);
            mysql.disconnect();
            client->shutdown();
            client->close();
            return {};
        }
    }
    user = mysql.getUserInfo(user.perm.id);
    mysql.disconnect();
    return user;
}

void cmdThread(FTPServer::Client *me, TcpSocket *client, std::string ip) {
    printf("thread started\n");
    std::string reply;
    Request request;
    std::string tmp;
    FTPServer::Client client_info{};

    client_info.cmdSocket = client;
    client_info._type = FTPServer::Client::ASCII;

    reply = "220 Welcome to FTP server by Grigoriy!\r\n";
    printf("< %s", reply.c_str());
    client->send(reply, 0);

    MySqlClient::DBUser user;
    user = auth(client);

    printf("\nStart communication\n");
    bool quit = false;

    std::string pwd = user.homedir;
    FileExplorer fe = FileExplorer(user.homedir);
    while(!quit) {
        tmp = client->recv();
        if (tmp.empty()) {
            print_error("E: empty request ");
            break;
        }
        request = tmp;
        printf("> %s", tmp.c_str());
        SWITCH(request.command().c_str()) {
            CASE("TYPE"):
                client_info._type = request.arg() == "I" ? FTPServer::Client::ASCII : FTPServer::Client::BIN;
                reply = std::string("200 Type ") + (request.arg() == "I" ? "ASCII" : "BINARY") +  "\r\n";
            break;
            CASE("NOOP"):
            reply = "200 Command OK\r\n";
            break;
            CASE("SYST"):
            reply = "215 UNIX Type: L8. Remote system type is UNIX.\r\n";
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
            if (fe.mkdir(request.arg()))
                reply = "230 Created dir " + request.arg() + "\r\n";
            else
                reply = "530 Error creating dir\r\n";
        }
            break;
            CASE("RMD"):
        {
            if (fe.rmdir(request.arg()))
                reply = "250 Directory removed\r\n";
            else
                reply = "530 Error creating folder\r\n";
        }
            break;
            CASE("SIZE"):
        {
            uint64_t size = fe.size(request.arg());
            if (size != -1)
                reply = "230 Size = " + std::to_string(size) + "\r\n";
            else
                reply = "530 Error getting size of file " + request.arg() + "\r\n";
        }
            break;
            CASE("DELE"):
            if (fe.rm(request.arg()))
                reply = "250 File " + request.arg() + "deleted\r\n";
            else
                reply = "530 Error deleting file\r\n";
            break;
            CASE("PASV"): {
                int *p = new int[2];
                if (pipe(p) == -1) {
                    print_error("E: pipe failed");
                    reply = "550 Error on the server\r\n";
                    break;
                } else reply = "";
                client_info.dt_info = new DataThread(client, user.homedir, p);
                memcpy(client_info.pipe, p, 2 * sizeof(int));
                // start_passive thread
                client_info.dt = new Thread<void, DataThread *, std::string, bool>{DataThread::run, client_info.dt_info, ip, false};
                }
            break;
            CASE("PORT"): {
                int *p = new int[2];
                if (pipe(p) == -1) {
                    print_error("E: pipe failed");
                    reply = "550 Error on the server\r\n";
                    break;
                } else reply = "200 Command okay.\r\n";
                client_info.dt_info = new DataThread(client, user.homedir, p);
                memcpy(client_info.pipe, p, 2 * sizeof(int));
                // start_active thread
                client_info.dt = new Thread<void, DataThread *, std::string, bool>{DataThread::run, client_info.dt_info, ip, true};
                }
            break;
            CASE("CDUP"):
                reply = "200 Changed directory to " + fe.cd_up() + "\r\n";
            break;
            CASE("CWD"):
                if (fe.cd(request.arg()))
                    reply = "200 Changed directory to " + fe.pwd() + "\r\n";
                else
                    reply = "400 Unknown directory " + request.arg() + "\r\n";
            break;
            CASE("LIST"): {
                std::string data = "SEND LIST\r\n";
                if (write(client_info.pipe[1], data.c_str(), data.size()) == -1){
                    print_error("E: write to pipe");
                    reply = "500 Error on the server\r\n";
                }
                else reply = "125 Transfer starting\r\n";
                client->send(reply);
                client_info.dt->join();
                reply = "250 Data transfered\r\n";
            }
            break;
            CASE("RETR"):
                if (client_info.dt_info != nullptr) {
                    std::string command = "SEND " + request.arg() + "\r\n";
                    if (write(client_info.pipe[1], command.c_str(), command.size()) == -1){
                        reply = "150 In progress\n";
                    } else
                        reply = "500 Error on the server\r\n";
                } else
                    reply = "451 Requested action aborted: First send PORT or PASV command\r\n";
            break;
            CASE("STOR"):
            if (client_info.dt_info != nullptr) {
                std::string command = "RECV " + request.arg() + "\r\n";
                if (write(client_info.pipe[1], command.c_str(), command.size()) != -1){
                    reply = "150 In progress\n";
                } else
                    reply = "500 Error on the server\r\n";
            } else
                reply = "451 Requested action aborted: First send PORT or PASV command\r\n";
            break;
            default:
                reply = "502 Command not implemented .\r\n";
            break;
        }

        if (!reply.empty()) {
            printf("< %s", reply.c_str());
            if (client->send(reply, 0) == 0){
                print_error("E: send failed");
                quit = true;
            }
        } else printf("pasv mode??\n");

    }

    printf("Close socket\n");
    client->shutdown();
    client->close();
    me->active = false;
}


void FTPServer::connection_handler(TcpSocket *sock) {
    printf("Connection accepted\n");
    Client *client = new Client;
    client->cmdSocket = sock;
    auto ptr = new Thread<void, Client*, TcpSocket*, std::string>{cmdThread, client, sock, ip};
    client->cmd = ptr;
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