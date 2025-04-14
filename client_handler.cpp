#include "client_handler.h"
#include <iostream>

ClientHandler::ClientHandler(SOCKET socket): clientSocket(socket), serverState(ServerState::INITIAL), isSession(true) {}

void ClientHandler::operator()()
{
    while (isSession)
    {
        int command = receiveCommand();

        if (command == CONFIG)
        {
            handleConfig();
        }
        else
        {
            std::cout << "Command not recognized" << std::endl;
            sendRespond(FAILED);
        }
    }
    std::cout << "Client disconnected" << std::endl;
    closesocket(clientSocket);
}

void ClientHandler::handleConfig()
{
    if(serverState == ServerState::INITIAL)
    {
        serverState = ServerState::CONFIGURED;
        // isSession = false;
        sendRespond(CONFIG_OK);
    }
    else
    {
        sendRespond(FAILED);
    }
}

int ClientHandler::receiveCommand()
{
    int networkCommand;
    int bytesReceived = recv(clientSocket, (char*)&networkCommand, sizeof(networkCommand), 0);
    if (bytesReceived == SOCKET_ERROR)
    {
        std::cerr << "Error receiving command or client disconnected.\n";
        isSession = false;
        return -1;
    }

    return ntohl(networkCommand);
}

void ClientHandler::sendRespond(ServerRespond response)
{
    int message = htonl(static_cast<int>(response));
    send(clientSocket, (char*)&message, sizeof(message), 0);
}
