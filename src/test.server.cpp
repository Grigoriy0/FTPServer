#include "request.h"
#include "TcpSocket.h"
#include "MySqlClient.h"
#include "DataThread.h"
#include "Thread.h"

enum transfer_t{ASCII, BIN};

namespace global {
    std::vector<DataThread *> dt_threads_info;
    std::vector<Thread<void, DataThread *> *> dt_threads;
    std::vector<Thread<void, TcpSocket*> *> cmd_threads;
}

struct Client{
  DataThread *dt_info;
  Thread<void, DataThread*> dt;

};

MySqlClient::DBUser auth(TcpSocket *client) {
    request buffer;
    std::string tmp;
    std::string reply;

    buffer = tmp = client->recv();
    printf("> %s\n", tmp.c_str());

    while(buffer.command() != "USER") {
        reply = "130 Sign in first\t\n";
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
        reply = "530 Sorry, error on the database server\t\n";
        printf("< %s", reply.c_str());
        client->send(reply);
        mysql.disconnect();
        client->close();
        return {};
    }
    else {
        if (user.uname != "anonymous")
            reply = "331 Password required\t\n";
        else
            reply = "230 OK\t\n";
        printf("< %s", reply.c_str());
        client->send(reply);

    }

    printf("Mysql connected\n");

    if (user.uname != "anonymous") {
        buffer = tmp = client->recv();
        printf("> %s\n", tmp.c_str());
        user.perm.id = mysql.auth(user.uname, buffer.arg());

        if (user.perm.id > 0) {
            reply = "230 Log in as user " + user.uname + "\t\n";
            printf("< %s", reply.c_str());
            client->send(reply);

        }
        else {
            reply = "130 Unknown user " + user.uname + "\t\n";
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

void handler(int sig){}

void cmdThread(TcpSocket *client) {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGPIPE);
    sigaction(SIGPIPE, &act, nullptr);
    sigprocmask(SIG_BLOCK, &act.sa_mask, nullptr);

    std::string reply;
    reply = "200 Hello World!\t\n";
    printf("< %s", reply.c_str());
    client->send(reply);
    int dt_thread_index;
    request buffer;
    std::string tmp;

    MySqlClient::DBUser user = auth(client);
    
    printf("Start communication\n");
    bool quit = false;
    transfer_t type = ASCII;
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
        SWITCH(buffer.command().c_str()){
            CASE("NOOP"):
                reply = "200 OK\t\n";
                break;
            CASE("SYST"):
                reply = "215 Unix\t\n";
                break;
            CASE("STAT"):
                reply = std::string("211 Logged in ") + user.uname + "\n"
                    + "211 End of status\t\n";
                break;
            CASE("QUIT"):
                reply = "221 Goodbye\t\n";
                quit = true;
                break;
            CASE("HELP"):
                reply = "500 I don't have man for this command\t\n";
                break;
            CASE("PWD"):
                reply = "257 \"" + fe.pwd() + "\" is the current directory\t\n";
                break;
            CASE("PASV"):
            {
                int p[2];
                if (pipe(p) == -1) {
                    print_error("E: pipe failed");
                    reply = "550 Error on the server\t\n";
                    break;
                }
                else reply = "100 init session\t\n";
                // start thread
                DataThread *new_dt = new DataThread(client, nullptr, p[0]);
                global::dt_threads_info.push_back(new_dt);
                global::dt_threads.push_back(new Thread<void, DataThread*>{DataThread::run, new_dt});
            }
            break;
            CASE("MKD"):
                if (fe.mkdir(buffer.arg()))
                    reply = "230 Created dir " + buffer.arg() + "\t\n";
                else
                    reply = "500 Error creating dir\t\n";
            break;
            CASE("RMD"):
                if (fe.rmdir(buffer.arg()))
                    reply = "230 Folder was removeed\t\n";
                else
                    reply = "500 Error creating folder\t\n";
            break;
            CASE("SIZE"):
            {
                uint64_t size = fe.size(buffer.arg());
                if (size != -1)
                    reply = "230 Size = " + std::to_string(size) + "\t\n";
                else
                    reply = "500 Error getting size of file " + buffer.arg() + "\t\n";
            }
            break;
            CASE("DELE"):
                if (fe.rm(buffer.arg()))
                    reply = "230 File " + buffer.arg() + "deleted\t\n";
                else
                    reply = "530 Error deleting file\t\n";
            break;
            default:
                quit = true;
                reply = "500 Unknown command\t\n";
                break;
        }
        printf("< %s", reply.c_str());
        client->send(reply);

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

    unsigned short port = 2000;
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
