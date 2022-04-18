#include "Server.hpp"

Server::Server(char *port, char *bd_ip, char *bd_prot) {
	for (int i = 0; port[i]; ++i) {
		if (!isdigit(port[i]))
			throw std::runtime_error("Порт приложения может содержать только цифры.");
	}
    for (int i = 0; bd_prot[i]; ++i) {
		if (!isdigit(bd_prot[i]))
			throw std::runtime_error("Порт субд может содержать только цифры.");
	}
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = htonl(INADDR_ANY);
    _address.sin_port = htons(atoi(port));

    _address_bd.sin_family = AF_INET;
    _address_bd.sin_port = htons(atoi(port));
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