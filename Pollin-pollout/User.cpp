#include "User.hpp"

User::User(int sc, sockaddr_in *addr) {
    _socket = sc;
    _len_req = 0;
    _send_res = 0;
    _send_req = 0;
    _isQuery = 0;

    _db_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(connect(_db_socket, (struct sockaddr *)addr, sizeof(*addr)) < 0) 
        throw  std::runtime_error("Сервер СУБД недоступен");
}

void User::addReq(char *buffer, int nbytes) {
    _req.append(buffer, nbytes);
    // if (_req[0] == 'Q') {
    //     if (_req.length() >= 5)
    //     {
    //         _len_req = int((unsigned char)(_req[1]) << 24 |
    //                     (unsigned char)(_req[2]) << 16 |
    //                     (unsigned char)(_req[3]) << 8 |
    //                     (unsigned char)(_req[4]));
    //     }
    // }
}

void User::addRes(char *buffer, int nbytes) {
    _res.append(buffer, nbytes);
}

bool User::getQuery() {
    return _isQuery;
}
void User::setQuery(bool status) {
    _isQuery = status;
}

long User::getReqLen() {
    return _len_req;
}

std::string &User::getReq(){
    return _req;
}

std::string &User::getRes(){
    return _res;
}

int User::get_dbSc() {
    return _db_socket;
}

int User::get_userSc() {
    return _socket;
}

long User::getSendReq() {
    return _send_req;
}

long User::getSendRes() {
    return _send_res;
}

void User::setSendReq(long len) {
    _send_req = len;
}

void User::setSendRes(long len) {
    _send_res = len;
}