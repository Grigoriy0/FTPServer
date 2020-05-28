

#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>


#include "TcpSocket.h"
#include "FileExplorer.h"
#include "DataThread.h"
#include "FTPServer.h"
#include "MySqlClient.h"
#include "Request.h"
#include "Client.h"


FTPServer::FTPServer(cstring root, uint16_t port, cstring ip, int max_conn) :
port(port), root_dir(root), max_connections(max_conn) {
    listening = new TcpSocket();
    if(!listening->bind(port)) {
        print_error("E: cannot bind port to listening socket\nExit\n");
        listening->close();
        exit(2);
    }
    listening->listen(max_connections);
    connections = 0;
    if (ip.empty())
        get_my_ip();
    else {
        this->ip = ip;
        printf("IPv4 %s ", ip.c_str());
    }
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
        client->send(reply);

        request = tmp = client->recv();
        if (tmp.empty()){
            MySqlClient::DBUser empty{};
            empty.id = 0;
            return empty;
        }
        printf("> %s\n", tmp.c_str());
    }


    MySqlClient::DBUser user;
    user.uname = request.arg();

    MySqlClient mysql(getenv("FTP_DB_NAME"));
    if (!mysql.connect(getenv("FTP_DB_USER"), getenv("FTP_DB_PASS"))) {
        print_error(mysql.last_error().c_str());
        reply = "530 Sorry, error on the database server\r\n";
        printf("< %s", reply.c_str());
        client->send(reply, 0);
        mysql.disconnect();
        client->shutdown();
        client->close();
        MySqlClient::DBUser empty{};
        empty.id = 0;
        return empty;
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
        user.id = mysql.auth(user.uname, request.arg());

        if (user.id > 0) {
            reply = "230 Log in as user " + user.uname + "\r\n";
            printf("< %s", reply.c_str());
            client->send(reply, 0);

        }
        else {
            reply = "530 Auth error. Bye " + user.uname + "\r\n";
            printf("> %s", reply.c_str());
            client->send(reply, 0);
            mysql.disconnect();
            client->shutdown();
            client->close();
            return {};
        }
    }
    user = mysql.getUserInfo(user.id);
    mysql.disconnect();
    return user;
}

void cmdThread(Client *me, std::string ip, std::string root) {
    printf("thread started\n");
    std::string reply;
    Request request;
    std::string tmp;

    reply = "220 Welcome to FTP server by Grigoriy!\r\n";
    me->send_reply(reply);

    MySqlClient::DBUser user;
    user = auth(me->cmdSocket);
    me->fe = new FileExplorer(root + user.homedir);
    me->name = user.uname;
    bool quit = false;

    std::string pwd = user.homedir;
    while(!quit) {
        tmp = me->cmdSocket->recv();
        if (tmp.empty()) {
            printf("W: empty request. Exit\n");
            break; //
        }
        request = tmp;
        printf("%s> %s", user.uname.c_str(), tmp.c_str());
        SWITCH(request.command().c_str()) {
            CASE("TYPE"):me->_type = request.arg() == "A" ? Client::ASCII : Client::BIN;
                reply = std::string("200 Type ") + (request.arg() == "A" ? "ASCII" : "BINARY") + "\r\n";
                break;
            CASE("NOOP"):reply = "200 Command OK\r\n";
                break;
            CASE("SYST"):reply = "215 UNIX Type: L8. Remote system transfer type is UNIX.\r\n";
                break;
            CASE("STAT"):reply = std::string("211 Logged in ") + user.uname + "\n" + "211 End of status\r\n";
                break;
            CASE("QUIT"):reply = "221 Goodbye\r\n";
                quit = true;
                break;
            CASE("HELP"):reply = "500 I don't have man for this command\r\n";
                break;
            CASE("PWD"):reply = "257 \"" + me->fe->pwd() + "\" is the current directory\r\n";
                break;
            CASE("CDUP"):reply = "200 Changed directory to " + me->fe->cd_up() + "\r\n";
                break;
            CASE("MKD"):
                if (me->fe->mkdir(request.arg()))
                    reply = "230 Created dir " + request.arg() + "\r\n";
                else
                    reply = "530 Error creating dir\r\n";
                break;
            CASE("RMD"):
                if (me->fe->rmdir(request.arg()))
                    reply = "250 Directory removed\r\n";
                else
                    reply = "530 Error removing folder\r\n";
                break;
            CASE("SIZE"): {
                uint64_t size = me->fe->size(request.arg());
                if (size != -1)
                    reply = "230 Size = " + std::to_string(size) + "\r\n";
                else
                    reply = "530 Error getting size of file " + request.arg() + "\r\n";
                }
                break;
            CASE("DELE"):
                if (me->fe->rm(request.arg()))
                    reply = "250 File " + request.arg() + " deleted\r\n";
                else
                    reply = "530 Error deleting file\r\n";
                break;
            CASE("PASV"):
                reply = me->init_passive_thread(ip);
                break;
            CASE("PORT"):
                reply = me->init_active_thread(request.arg());
                break;
            CASE("CWD"):
                if (request.arg().substr(0, 2) == "..") {
                    reply = "200 Changed directory to " + me->fe->cd_up() + "\r\n";
                    break;
                }
                if (me->fe->cd(request.arg()))
                    reply = "200 Changed directory to " + me->fe->pwd() + "\r\n";
                else
                    reply = "400 Unknown directory " + request.arg() + "\r\n";
                break;
            CASE("LIST"):
                if (write(me->_pipe[1], "LIST\r\n", 7) != -1)
                    reply = "125 Transfer starting\r\n";
                else {
                    print_error("E: write to _pipe");
                    reply = "500 Error on the server\r\n";
                }
                break;
            CASE("RETR"):
                if (!me->dt_info || me->dt_info->works()) {
                    if (me->send_command("SEND " + request.arg() + "\r\n")) {
                        reply = "125 In progress\r\n";
                    } else {
                        print_error("E: write to _pipe failed\r\n");
                        reply = "500 Error on the server\r\n";
                    }
                } else
                    reply = "451 Requested action aborted: First send PORT or PASV command\r\n";
                break;
            CASE("STOR"):
                if (!me->dt_info || me->dt_info->works()) {
                    if (me->send_command("RECV " + request.arg() + "\r\n")) {
                        reply = "125 In progress\r\n";
                    } else
                        reply = "500 Error on the server\r\n";
                } else
                    reply = "451 Requested action aborted: First send PORT or PASV command\r\n";
                break;
            CASE("ABOR"):
                if (!me->dt_info || !me->dt_info->works()) {
                    if (me->dt->cancel()) {
                        reply = "220 data transfer aborted\r\n";
                    }else {
                        print_error("error file abort data transferring");
                        reply = "500 Aborting error\r\n";
                        break;
                    }
                } else
                    reply = "451 Requested action aborted: Nothing to abort\r\n";
                break;
            default:reply = "502 Command not implemented .\r\n";
                break;
        }

        if (!reply.empty()) {
            // PASV and PORT command
            if (me->send_reply(reply) == false) {
                if (errno == ECONNRESET)
                    printf("Client reset connection\r\n");
                else {
                    print_error("E: send failed");
                }
                quit = true;
            }
        }
    }

    printf("Close connection with %s\n", user.uname.c_str());
    me->send_command("STOP\r\n"); // if thread wait command
    me->cmdSocket->shutdown();
    me->dt->cancel(); // if thread does not wait
    me->cmdSocket->close();
    me->active = false;
}


void FTPServer::connection_handler(TcpSocket *sock) {
    printf("Connection accepted\n");
    Client *client = new Client(sock);
    auto ptr = new Thread<void, Client*, std::string, std::string>{cmdThread, client, ip, root_dir};
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