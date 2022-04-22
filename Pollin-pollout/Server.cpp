#include "Server.hpp"

std::stringstream Server::datetime(){
    auto now = std::chrono::system_clock::now();
    auto UTC = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream datetime;
    datetime << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return datetime;
}

Server::Server(char *port, char *bd_ip, char *bd_prot) {
	for (int i = 0; port[i]; ++i) {
		if (!isdigit(port[i]))
			throw std::runtime_error("Порт сервера может содержать только цифры.");
	}
    for (int i = 0; bd_prot[i]; ++i) {
		if (!isdigit(bd_prot[i]))
			throw std::runtime_error("Порт субд может содержать только цифры.");
	}
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = htonl(INADDR_ANY);
    _address.sin_port = htons(atoi(port));

    _address_bd.sin_family = AF_INET;
    _address_bd.sin_port = htons(atoi(bd_prot));
    if (!strcmp(bd_ip, "localhost"))
        inet_aton("127.0.0.1", &_address_bd.sin_addr);
    else if (!inet_aton(bd_ip, &_address_bd.sin_addr)){
        throw std::runtime_error("Неверный ip адресс.");
    }

    _file_name = "proxy_log[";
    _file_name += datetime().str();
    std::replace(_file_name.begin(), _file_name.end(), ':', '-');
    std::replace(_file_name.begin(), _file_name.end(), ':', '-');
    _file_name += "].log";
}

void    Server::start() {
	_sc = socket(AF_INET, SOCK_STREAM, 0);
	if (_sc < 0)
		throw std::runtime_error((std::string)__func__ + ": socket()");
 
 	int enable = 1;
	if (setsockopt(_sc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		close(_sc);
		throw std::runtime_error((std::string)__func__ + ": setsockopt()");
	}

	if (fcntl(_sc, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error((std::string)__func__ + ": fcntl()");

	if (bind(_sc, reinterpret_cast<struct sockaddr*>(&_address), sizeof(_address)) < 0) {	
		close(_sc);
		throw std::runtime_error((std::string)__func__ + ": bind()");
	}

	if (listen(_sc, SOMAXCONN) < 0) {
		close(_sc);
		throw std::runtime_error((std::string)__func__ + ": listen()");
	}
    struct pollfd tmp;
    tmp.fd = _sc;
    tmp.events = POLLIN;
    tmp.revents = 0;
    _fds.push_back(tmp);
    printf("Прокси сервер запущен\n");
}

void Server::poll_in(pollfdType::iterator &it)
{
    it->revents = 0;
    sockaddr_in	addr;
    socklen_t addr_len = sizeof(addr);
    int user_fd = accept(it->fd, reinterpret_cast<struct sockaddr*>(&addr), &addr_len);
    if (user_fd < 0)
        std::cout << "Сокет не откры для пользователя" << std::endl;
    else {
        char buffer[16];
        inet_ntop( AF_INET, &addr.sin_addr, buffer, sizeof(buffer));

        User *new_user = new User(user_fd, &_address_bd);

        struct pollfd tmp;
        tmp.fd = user_fd;
        tmp.events = POLLIN;
        tmp.revents = 0;

        if (fcntl(user_fd, F_SETFL, O_NONBLOCK) < 0) {
            std::cout << "Ошибка fcntl() пользователь: " << user_fd << " адрес: " << buffer << std::endl;
            close(user_fd);
            return;
        }

        int enable = 1;
        if (setsockopt(user_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
            std::cout << "Ошибка setsockopt() пользователь: " << user_fd << " адрес: " << buffer << std::endl;
            close(user_fd);
            return;
		}

        _fds.push_back(tmp);
        tmp.fd = new_user->get_dbSc();
        tmp.events = POLLIN;
        _fds.push_back(tmp);
        _users.insert(std::pair<int, User *>(user_fd, new_user));
        _users.insert(std::pair<int, User *>(new_user->get_dbSc(), new_user));
        std::cout << "Соединение установлено, пользователь: " << user_fd << " адрес: " << buffer << std::endl;
	}
}

void Server::write_log(char *buffer, int nbytes, bool status) {
    static bool time = 0;
    std::ofstream out(_file_name.c_str(), std::ios::app);
    if (!out) {
        std::cerr << "Ошибка открытия файла " << _file_name << std::endl;
        return;
    }
    if (time == 0) {
        out << "[" << datetime().str() << "]:" << std::endl;
        time = 1;
        out.write(buffer+5, nbytes -6);
    }
    else
        out.write(buffer, nbytes);
    if (status == 1)
        time = 0;
    out.close();
}

bool Server::send_to(int sc, char *buff, int nbytes, bool client) {
    int ret;
    int exit = 0;
    std::string msg;
    
    while (exit < nbytes) {
        ret = send(sc, buff + exit, nbytes - exit, 0);
        if (ret == -1) {
            if (client)
                msg = "Ошибка отправки клиенту, соединение закрыто.";
            else
                msg = "Ошибка отправки серверу СУБД, соединение закрыто.";
            //write_log(msg);
            return (1);
        }
        exit += ret;
    }
    return (0);
}

bool Server::poll_user(pollfdType::iterator &it)
{
    it->revents = 0;
    User *itu = _users[it->fd];
    std::string msg;
    auto iter = it;
    char buffer[MAX_BUFFER_RECV];
    int nbytes = recv(it->fd, buffer, MAX_BUFFER_RECV, 0);
    std::cout << "priwli v Recv : " << it->fd  << " buff: " << nbytes << std::endl;
    if (nbytes < 0) {
        msg = "Ошибка чтения с сокета, соединение закрыто.";
        // /write_log(msg);
        return 0;
    } 
    else {
        if (it->fd == itu->get_userSc()) {
            if (buffer[0] == 'Q')
                itu->setQuery(1);
            if (itu->getQuery() == 1) {
                if (nbytes < MAX_BUFFER_RECV) {
                    write_log(buffer, nbytes, 1);
                    itu->setQuery(0);
                }
                else
                    write_log(buffer, nbytes, 0);
            }
            if (nbytes == 0)
                return 0;
            itu->addReq(buffer,nbytes);
            //if (nbytes < MAX_BUFFER_RECV)
            ++iter;
            iter->events = POLLIN | POLLOUT;
            std::cout << "ставим сервер в пулаут: "<< iter->fd << std::endl;
        }
        else {
            itu->addRes(buffer,nbytes);
            //if (nbytes < MAX_BUFFER_RECV )
            --iter;
            iter->events = POLLIN | POLLOUT;
             std::cout << "ставим пользователя  в пулаут" << std::endl;
        }
    }
    return 1;
}

bool Server::poll_out(pollfdType::iterator &it) {
    it->revents = 0;
    User *itu = _users[it->fd];
    long ret;
    //std::cout <<  "Хочу отправить на: " << it->fd << std::endl;
    if (it->fd == itu->get_userSc() && itu->getRes().size()) {

        ret = send(it->fd, itu->getRes().c_str() + itu->getSendRes(), itu->getRes().size() - itu->getSendRes(), 0);
        if (ret < 0)
            return 0;
        itu->setSendRes(itu->getSendRes() + ret);
        if (itu->getRes().size() == itu->getSendRes()){
            itu->getRes().clear();
            itu->setSendRes(0);
                    std::cout << "Отправка пользователю, пользователь в пулин" << std::endl;
            it->events = POLLIN;
        }
    }
    else if (it->fd == itu->get_dbSc() && itu->getReq().size()) {
        ret = send(it->fd, itu->getReq().c_str() + itu->getSendReq(), itu->getReq().size() - itu->getSendReq(), 0);
        if (ret < 0)
            return 0;
        itu->setSendReq(itu->getSendReq() + ret);
        if (itu->getReq().size() == itu->getSendReq()){
            itu->getReq().clear();
            itu->setSendReq(0);
                        std::cout << "Отправка серверу, сервер в пулин" << std::endl;
            it->events = POLLIN;
        }
    }
    return 1;
}

void Server::loop() {
    while (true)
    {
        if (poll(&(_fds.front()), _fds.size(), 0) < 0) {
            std::cout << (std::string)__func__ + ": poll()" << std::endl;
            continue;
        }

        for (pollfdType::iterator it = _fds.begin(); it != _fds.end();)
        {
            if (it->revents == 0){
                ++it;
                continue;
            }
            if (it->revents & POLLIN && it->fd == _sc) {
                poll_in(it);
                break;
            }
            else if (it->revents & POLLIN) {
                if (!poll_user(it)){
                    if (_users[it->fd]->get_dbSc() == it->fd)
                        --it;
                    delete(_users[it->fd]);
                    _users.erase(it->fd);
                    close(it->fd);
                    it = _fds.erase(it);
                    _users.erase(it->fd);
                    close(it->fd);
                    it = _fds.erase(it);
                    continue;
                }
            }
            else if (it->revents & POLLOUT) {
                if (!poll_out(it)) {
                    if (_users[it->fd]->get_dbSc() == it->fd)
                        --it;
                    delete(_users[it->fd]);
                    _users.erase(it->fd);
                    close(it->fd);
                    it = _fds.erase(it);
                    _users.erase(it->fd);
                    close(it->fd);
                    it = _fds.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }
}