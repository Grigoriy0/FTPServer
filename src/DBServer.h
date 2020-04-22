//
// Created by grigoriy on 23.02.20.
//

#ifndef DBSERVER_H
#define DBSERVER_H
#include <string>
#include <mysql++.h>

#include "defines.h"
class DBServer {
public:
    explicit DBServer(cstring database);

    bool connect(cstring user, cstring passw);

    struct DBUser{
        struct permissions {
            uint32_t id;
            bool isroot;
        } perm;
        std::string homedir;
        std::string uname;
    };

    int auth(cstring uname, cstring pass);

    std::string last_error() const;

    DBUser getUserInfo(uint32_t id);

    void disconnect();
private:
    std::string error;
    std::string db_name;
    mysqlpp::Connection conn;

};


#endif //DBSERVER_H
