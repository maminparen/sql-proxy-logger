#include "User.hpp"

User::User(int sc, sockaddr_in *addr) {
    _socket = sc;
    _msg = "";

    _db_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(connect(_db_socket, (struct sockaddr *)addr, sizeof(*addr)) < 0) 
        throw  std::runtime_error("Сервер СУБД недоступен");
}

void User::check_req(char *buffer, int nbytes) {
    _msg += buffer;
}

std::string &User::getMsg(){
    return _msg;
}

int User::get_dbSc() {
    return _db_socket;
}

int User::get_userSc() {
    return _socket;
}