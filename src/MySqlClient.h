#ifndef MY_SQL_CLIENT_H
#define MY_SQL_CLIENT_H
#include <string>
#include <mysql++.h>

#include "defines.h"
class MySqlClient {
public:
    explicit MySqlClient(cstring database);

    bool connect(cstring user, cstring passw);

    struct DBUser{
        int id;
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


#endif //MY_SQL_CLIENT_H
