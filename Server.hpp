#pragma once

#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <numeric>

class Server {
    struct sockaddr_in  		_address;
    struct sockaddr_in  		_address_bd;
    std::string                 _file_name;
private:
    Server();
public:
    Server(char *port, char *bd_ip, char *bd_prot);
};