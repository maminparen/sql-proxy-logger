#include "../include/Server.hpp"

using namespace std;

void signal_handler(int signal) {
    (void)signal;
    cout << "Сервер остановлена." << endl;
    exit(0);
}

int main (int argc, char **argv) {

    if (argc != 4) {
        cerr << "Для запуска программы необходимо указать используемый порт, адрес субд, порт субд.\n" + (string)argv[0] + " <порт> <адрес субд> <порт субд>" << endl;
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTSTP, signal_handler);

    try {
        Server server(argv[1], argv[2], argv[3]);
        server.start();
        server.loop();
    }
    catch (std::exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
        return 1;
    }
    return 0;
}