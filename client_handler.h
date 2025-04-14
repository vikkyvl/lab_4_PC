#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <winsock2.h>
#include <vector>
#include "commands.h"

class ClientHandler
{
private:
    SOCKET clientSocket;
    std::vector<std::vector<int>> matrix;
    ServerState serverState;
    bool isSession;

public:
    explicit ClientHandler(SOCKET clientSocket);
    void operator()();

    void handleConfig();

    void sendRespond(ServerRespond response);
    int receiveCommand();
};

#endif //CLIENT_HANDLER_H
