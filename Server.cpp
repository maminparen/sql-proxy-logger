#include "Server.hpp"

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
    auto now = std::chrono::system_clock::now();
    auto UTC = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream datetime;
    datetime << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    _file_name += datetime.str();
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
        _fds.push_back(tmp);
		_users.insert(std::pair<int, User *>(user_fd, new_user));
        _users.insert(std::pair<int, User *>(new_user->get_dbSc(), new_user));
        std::cout << "Соединение установлено, пользователь: " << user_fd << " адрес: " << buffer << std::endl;
	}
}

int Server::poll_user(pollfdType::iterator &it)
{
	it->revents = 0;
	User *itu = _users[it->fd];

	char buffer[MAX_BUFFER_RECV + 1];
	int nbytes = recv(it->fd, buffer, MAX_BUFFER_RECV, 0);
	if (nbytes <= 0) 
		return 0;
	else {
        buffer[nbytes] = '\0';
		itu->check_req(buffer, nbytes);
        if (nbytes != MAX_BUFFER_RECV) {
            if (it->fd == itu->get_dbSc())
                send(itu->get_userSc(), itu->getMsg().c_str(),itu->getMsg().length() ,0);
            else
                send(itu->get_dbSc(), itu->getMsg().c_str(),itu->getMsg().length() ,0);
            itu->getMsg().clear();
        }
    }
    return 1;
}

int Server::poll_out(pollfdType::iterator &it) {
    
    return 1;
}

void Server::loop() {
    while (true)
	{
		if (poll(&(_fds.front()), _fds.size(), 0) < 0) {
			std::cout << (std::string)__func__ + ": poll()" << std::endl;
            continue;
        }

		for (pollfdType::iterator it = _fds.begin(), itend = _fds.end(); it != itend; ++it)
		{
			if (it->revents == 0)
				continue;

			if (it->revents & POLLIN && it->fd == _sc) {
				poll_in(it);
				break;
			}
			else if (!(it->revents & POLLIN && poll_user(it)) ){
                close(it->fd);
                int tmp = _users[it->fd]->get_dbSc();
                if (it->fd == tmp)
                    tmp = _users[it->fd]->get_userSc();
                close(tmp);
                delete(_users[it->fd]);
                _users.erase(it->fd);
                _users.erase(tmp);
                it = _fds.erase(it);
                for (auto iter = _fds.begin(); iter != _fds.end(); ++iter) {
                    if (iter->fd == tmp) {
                        _fds.erase(iter);
                        break;
                    }
                }
            }
		}
	}
}