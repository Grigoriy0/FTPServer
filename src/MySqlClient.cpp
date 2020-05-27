#include "MySqlClient.h"
#include <utility>
#include <cstring>

MySqlClient::MySqlClient(cstring database):
    db_name(std::move(database)), 
    conn(false){

    };

bool MySqlClient::connect(cstring user, cstring passw) {
    conn.connect(db_name.c_str(), "localhost", user.c_str(), passw.c_str());
    if (not conn.connected())
    {
        error = conn.error();
        return false;
    }
    return true;
}


int MySqlClient::auth(cstring uname, cstring pass) {
    if (uname == "anonymous")
        return 0;
    std::string stringQuery = std::string("SELECT id from users WHERE username"
                                          "= '" + uname + "' and password = '"+ pass + "';");
    mysqlpp::Query query = conn.query(stringQuery);

    mysqlpp::StoreQueryResult reply = query.store();
    if (not conn || reply.empty()){
        error = conn.error();
        return -1;
    }
    return std::stoi(reply[0][0].c_str());
}

std::string MySqlClient::last_error() const {
    return error;
}

MySqlClient::DBUser MySqlClient::getUserInfo(uint32_t id) {
    DBUser userInfo;

    std::string stringQuery =
            std::string("SELECT * FROM users WHERE id = ") + std::to_string(id) + ";";
    mysqlpp::StoreQueryResult reply = conn.query(stringQuery).store();
    if (not conn || reply.empty()){
        error = conn.error();
        return {};
    }
    userInfo.id = std::stoi(reply[0][0].c_str());
    userInfo.homedir = reply[0][3].c_str();
    userInfo.uname = reply[0][1].c_str();
    return userInfo;
}

void MySqlClient::disconnect()
{
    conn.disconnect();
}
