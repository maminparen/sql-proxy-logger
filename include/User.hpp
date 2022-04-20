#pragma once

#include "Server.hpp"

class User {
private:
    int _socket;
    int _db_socket;
    std::string _msg;
    User();
public:
    User(int sc, sockaddr_in *addr);
    void add_req(char *buffer, int nbytes);
    std::string &getMsg();
    int get_dbSc();
    int get_userSc();
};