#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "commands.h"

#pragma comment(lib, "Ws2_32.lib")

#define PORT 1234
#define SERVER_IP "127.0.0.1"

#define MIN 1
#define MAX 100

struct MatrixData
{
    int matrixSize;
    int threadsCount;
    std::vector<std::vector<int>> matrix;

    MatrixData(): matrixSize(0), threadsCount(0), matrix() {}

    MatrixData(const int matrixSize_, const int threadsCount_): matrixSize(matrixSize_), threadsCount(threadsCount_), matrix(matrixSize_, std::vector<int>(matrixSize_))
    {
        fillMatrix();
    }

    void printMatrix()
    {
        for (const auto& row : matrix)
        {
            for (const auto val : row)
            {
                std::cout << val << "\t";
            }
            std::cout << "\n";
        }
    }

private:
    static int getRandomNumber()
    {
        return MIN + (rand() % (MAX - MIN + 1));
    }

    void fillMatrix()
    {
        for (int i = 0; i < matrixSize; ++i)
        {
            for (int j = 0; j < matrixSize; ++j)
            {
                matrix[i][j] = getRandomNumber();
            }
        }
    }
};

void sendCommand(SOCKET socket, const ClientRequest command)
{
    int networkCommand = htonl(command);
    send(socket, (char*)&networkCommand, sizeof(networkCommand), 0);
}

void sendData(SOCKET socket, const MatrixData& data)
{
    int size, value, thread_N;
    size = htonl(data.matrixSize);
    send(socket, (char*)&size, sizeof(size), 0);

    for (int i = 0; i < data.matrixSize; i++)
    {
        for (int j = 0; j < data.matrixSize; j++)
        {
            value = htonl(data.matrix[i][j]);
            send(socket, (char*)&value, sizeof(value), 0);
        }
    }

    thread_N = htonl(data.threadsCount);
    send(socket, (char*)&thread_N, sizeof(thread_N), 0);
}

int receiveRespond(SOCKET socket)
{
    int networkResponse;
    int bytesReceived = recv(socket, (char*)&networkResponse, sizeof(networkResponse), 0);

    if (bytesReceived <= 0)
    {
        std::cerr << "Failed to receive response from server.\n";
        return -1;
    }

    int response = ntohl(networkResponse);
    return response;
}

void receiveData()
{

}

int main()
{
    srand(time(NULL));

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Error creating socket.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to server failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    int matrixSize, threadsCount;
    std::cout << "Enter matrix size: ";
    std::cin >> matrixSize;

    std::cout << "Enter number of threads: ";
    std::cin >> threadsCount;

    MatrixData data(matrixSize, threadsCount);
    // data.printMatrix();

    sendCommand(clientSocket, CONFIG);
    int message = receiveRespond(clientSocket);
    if(message != CONFIG_OK)
    {
        std::cerr << "Config error: " << message << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
