#include "request.h"
#include "tcpsocket.h"
#include "MySqlClient.h"

enum transfer_t{ASCII, BIN};
int connections = 0;

void cmdThread(TcpSocket client){
    client.send("200 Hello World!\t\n");
    request buffer;
    
    printf("%s %s\n", buffer.command().c_str(), buffer.arg().c_str());
    buffer = client.recv(BUF_SIZE);    
    while(buffer.command() != "USER"){
        client.send("510 Sign in first\t\n");
        buffer = client.recv(BUF_SIZE);
    }
    client.send("230 OK\t\n");
    printf("%s %s\n", buffer.command().c_str(), buffer.arg().c_str());
    MySqlClient::DBUser user;
    user.uname = buffer.arg();

    MySqlClient mysql("FTPServer");
    if (!mysql.connect("root", "2349914")){
        perror(mysql.last_error().c_str());
        client.send("130 Sorry, error on the database server\t\n");
        mysql.disconnect();
        client.close();
        return;
    }
    printf("Mysql connected\n");
    
    if (user.uname != "anonymous"){
        buffer = client.recv(BUF_SIZE);
        printf("Get user %s\n", buffer.arg().c_str()); 
        user.perm.id = mysql.auth(user.uname, buffer.arg());
        if (user.perm.id > 0){
            printf("OK\n");
            client.send("230 Log in as user " + user.uname + "\t\n");
            user = mysql.getUserInfo(user.perm.id);
        }
        else
        {
            printf("BAD\n");
            client.send("130 Unknow user " + user.uname + "\t\n");
            mysql.disconnect();
            client.close();
            return;
        }
    }
    else {
        user.perm.id = mysql.auth(user.uname, "");
        user = mysql.getUserInfo(user.perm.id);
    }
    mysql.disconnect();
    
    printf("Start communication\n");
    bool quit = true;
    transfer_t type = ASCII;
    std::string pwd = user.homedir;
    do{
        
        printf("%s %s\n", buffer.command().c_str(), buffer.arg().c_str());
        buffer = client.recv(BUF_SIZE);
        SWITCH(buffer.command().c_str()){
            CASE("NOOP"):
                client.send("200 OK\t\n");
                break;
            CASE("SYST"):
                client.send("215 Unix\t\n");
                break;
            CASE("STAT"):
                client.send(std::string("211 Logged in ") + user.uname + "\n"
                        + "TYPE: " + ((type==ASCII)?"ASCII\n":"BIN\n")
                        + "211 End of status\t\n");
                break;
            CASE("QUIT"):
                quit = true;
                break;
            CASE("HELP"):
                client.send("500 I don't have man for this command\t\n");
                break;
            CASE("PWD"):
                client.send("257 \"" + pwd + "\" is the current directory\t\n");
                break;
            CASE("TYPE"):
                client.send(std::string("200 Type set to ") + (buffer.arg()=="I"?"ASCII\t\n":"image/binary\t\n"));
                type = buffer.arg()=="I"? ASCII : BIN;
                break;
            default:
                client.send("500 Unknown command\t\n");
                break;
        }

    } while(quit == false);
    
    
    printf("Close socket\n");
    client.close();
    exit(0);
}
void cmd(TcpSocket*){}

#include"datathread.h"
void createNewSession(TcpSocket* cmdClient){
    Thread<void, TcpSocket*> cmdThread{cmd, cmdClient};
}

/*int main(int argc, char *argv[])
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
    do{
        newClient = serverSocket.accept();
        printf("Connection accepted\n");
        connections++;
        createNewSession(&newClient);
    }while(true);
    serverSocket.close();

	return 0;
}*/


int main(int argc, char*argv[]){
    printf("start\n");
    int fd[2];
    if (pipe(fd) == -1){
        print_error("pipe failed");
        return -1;
    }
    Thread<void, TcpSocket*,pthread_t, int*> th(data_thread, nullptr, pthread_self(), fd);
    //close(fd[0]);
    printf("write to pipe\n");
    std::string buf = "in.txt out.txt";
    if (write(fd[1], buf.c_str(), buf.size()) == -1){
        print_error("write(pipe) failed");
        return 2;
    }
    th.join();
    close(fd[1]);
    return 0;
}
