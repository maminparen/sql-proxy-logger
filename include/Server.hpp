#pragma once

#include <iostream>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <vector>
#include <string>
#include <algorithm>
#include <string.h>
#include <map>

#include "User.hpp"

#define MAX_BUFFER_RECV 1024

class User;

class Server {
    struct sockaddr_in                          _address;
    struct sockaddr_in                          _address_bd;
    std::string                                 _file_name;
    int                                         _sc;
    typedef std::vector<struct pollfd>	        pollfdType;
	pollfdType							        _fds;
    std::map <int, User *>                      _users;
private:
    Server();
public:
    Server(char *port, char *bd_ip, char *bd_prot);
    void                    start();
    void                    loop();
    void                    poll_in(pollfdType::iterator &it);
    bool                    poll_user(pollfdType::iterator &it);
    bool                    send_to(int sc, char *buff, int nbytes, bool client);
    void                    check_msg(std::string &msg);
    void                    write_log(std::string &msg);
    std::stringstream       datetime();
};