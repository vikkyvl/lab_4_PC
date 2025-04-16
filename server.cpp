#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include "client_handler.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_CLIENTS 2
std::atomic<int> activeClients(0);

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "Error creating socket.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Binding error.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listening error.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started on port " << PORT << "." << std::endl;

    while(true)
    {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Error accepting client connection.\n";
        }

        if (activeClients >= MAX_CLIENTS)
        {
            std::cerr << "Too many clients. Rejecting new client.\n";

            int busyResponse = htonl(BUSY);
            uint8_t type = COMMAND;
            uint32_t length = htonl(sizeof(int));

            send(clientSocket, (char*)&type, sizeof(type), 0);
            send(clientSocket, (char*)&length, sizeof(length), 0);
            send(clientSocket, (char*)&busyResponse, sizeof(busyResponse), 0);

            closesocket(clientSocket);
            continue;
        }

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);

        std::cout << "Client connected from IP: " << clientIp << " | Port: " << clientPort << std::endl;

        activeClients.fetch_add(1);
        std::cout << "New client connected. Active clients: " << activeClients.load() << std::endl;

        std::thread clientThread((ClientHandler(clientSocket, clientIp, clientPort)));
        clientThread.detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
