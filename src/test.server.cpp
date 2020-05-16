#include "request.h"
#include "TcpSocket.h"
#include "MySqlClient.h"
#include "DataThread.h"
#include "Thread.h"



namespace global {
    std::vector<Thread<void, TcpSocket*> *> cmd_threads;
}

struct Client {
    DataThread *dt_info;
    Thread<void, DataThread*, std::string> *dt;
    TcpSocket *cmdSocket;
    int pipe[2];

    bool send_command(cstring command){
        return write(pipe[1], command.c_str(), command.size()) != -1;
    }

};

MySqlClient::DBUser auth(TcpSocket *client) {
    request buffer;
    std::string tmp;
    std::string reply;

    buffer = tmp = client->recv();
    printf("> %s\n", tmp.c_str());

    while(buffer.command() != "USER") {
        reply = "130 Sign in first\r\n";
        printf("< %s", reply.c_str());
        client->send(reply);

        buffer = tmp = client->recv();
        printf("> %s\n", tmp.c_str());
    }


    MySqlClient::DBUser user;
    user.uname = buffer.arg();

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
    else {
        if (user.uname != "anonymous")
            reply = "331 Password required\r\n";
        else
            reply = "230 OK\r\n";
        printf("< %s", reply.c_str());
        client->send(reply);
    }

    printf("Mysql connected\n");

    if (user.uname != "anonymous") {
        buffer = tmp = client->recv();
        printf("> %s\n", tmp.c_str());
        user.perm.id = mysql.auth(user.uname, buffer.arg());

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
    request buffer;
    std::string tmp;
    Client client_info{};

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
                client_info.dt_info = new DataThread(client, &fe, p[0]);
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
            client->send(reply);
        }

    } while(!quit);

    printf("Close socket\n");
    client->close();
    exit(0);
}
void cmd(TcpSocket*){}

void createNewSession(TcpSocket* cmdClient){
    global::cmd_threads.push_back(new Thread<void, TcpSocket*>{cmdThread, cmdClient});
}

int main(int argc, char *argv[])
{
    TcpSocket serverSocket = TcpSocket();

    unsigned short port = std::atoi(argv[1]);
    if (!serverSocket.bind(port)){
        exit(-1);
    }

    if(!serverSocket.listen(1000)){
        exit(-1);
    }
    printf("Server listening to port %d\n", port);

    TcpSocket newClient;
    newClient = serverSocket.accept();
    printf("Connection accepted\n");
    createNewSession(&newClient);
    serverSocket.close();

    global::cmd_threads[0]->join();

	return 0;
}
