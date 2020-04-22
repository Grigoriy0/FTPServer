#include "functions.h"
#define bufSize = 256;

extern static int connections = 0;

void session(TcpSocket client){
    printf("Create new session\n");
    std::string buffer;
    client.send("200 Hello World!\t\n");
   
    buffer = client.recv(bufSize);    
    while(buffer.substr(0, 4) != "USER"){
        client.send("510 Sign in first\t\n");
        buffer = client.recv(bufSize);
    }
    client.send("230 OK\t\n");
    printf("Get user %s\n", buffer.substr(5, -1).c_str()); 
    DBServer::DBUser user;
    user.uname = buffer.substr(5, -1);
    DBServer mysql("FTPServer");
    if (!mysql.connect("root", "2349914")){
        perror(mysql.last_error().c_str());
        client.send("130 Sorry, error on the database server\t\n");
        mysql.disconnect();
        return;
    }
    printf("Mysql connected\n");
    if (user.uname != "anonymous"){
        buffer = client.recv(bufSize).substr(5, -1);
        printf("Trying to auth %s %s\n", user.uname.substr(0, user.uname.size() - 2).c_str(), buffer.substr(0, buffer.size() - 2).c_str());
        user.perm.id = mysql.auth(user.uname.substr(0, user.uname.size() - 2), buffer.substr(0, buffer.size() - 2));
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
    // TODO wait for SYST and other commands
    
    
    
    
    printf("Close socket\n");
    client.close();
    mysql.disconnect();
    exit(0);
}



int createNewSession(TcpSocket client){
    int pid = fork();
    if (pid == -1)return -1;
    if (pid != 0)return pid;
    session(client); 
    
    return 0;
}




int main(int argc, char *argv[])
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    
    sigprocmask(SIG_UNBLOCK, &set, NULL);

    struct sigaction child_act;
    child_act.sa_mask = set;
    child_act.sa_sigaction = childExitedHandler;
    child_act.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &child_act, NULL);

    TcpSocket serverSocket = TcpSocket();
    
    if (!serverSocket.bind(2000)){
        exit(-1);
    }

    if(!serverSocket.listen(1000)){
        exit(-1);
    }

    TcpSocket newClient;
    do{
        newClient = serverSocket.accept();
        printf("Connection accepted\n");
        connections++;
        createNewSession(newClient);
    }while(true);
    serverSocket.close();

	return 0;
}
