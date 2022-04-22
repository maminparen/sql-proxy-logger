#pragma once

#include "Server.hpp"

class User {
private:
    int                 _socket;
    int                 _db_socket;
    std::string         _req;
    long                _len_req;
    std::string         _res;
    long                _send_req;
    long                _send_res;
    bool                _isQuery;

    User();
public:
    User(int sc, sockaddr_in *addr);
    void addReq(char *buffer, int nbytes);
    long  getReqLen();
    std::string &getReq();
    void addRes(char *buffer, int nbytes);
    std::string &getRes();
    int get_dbSc();
    int get_userSc();
    long getSendReq();
    long getSendRes();
    void setSendReq(long len);
    void setSendRes(long len);
    void setQuery(bool status);
    bool getQuery();
};