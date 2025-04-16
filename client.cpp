#include <chrono>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <vector>
#include <cmath>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "commands.h"

#pragma comment(lib, "Ws2_32.lib")

#define PORT 8080
#define SERVER_IP "192.168.0.101"//"127.0.0.1"

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

void sendTLV(SOCKET socket, uint8_t type, const void* data, uint32_t length)
{
    send(socket, (char*)&type, sizeof(type), 0);

    uint32_t netLength = htonl(length);
    send(socket, (char*)&netLength, sizeof(netLength), 0);

    send(socket, (char*)data, length, 0);
}

void sendCommand(SOCKET socket, const ClientRequest command)
{
    int networkCommand = htonl(command);
    sendTLV(socket, COMMAND, &networkCommand, sizeof(networkCommand));
}

void sendData(SOCKET socket, const MatrixData& data)
{
    int totalElements = data.matrixSize * data.matrixSize;
    std::vector<int> matrixArray;
    matrixArray.reserve(totalElements);

    for (const auto& row : data.matrix)
    {
        for (int value : row)
        {
            matrixArray.push_back(htonl(value));
        }
    }

    sendTLV(socket, MATRIX, matrixArray.data(), totalElements * sizeof(int));

    int thread_N = htonl(data.threadsCount);
    sendTLV(socket, THREADS, &thread_N, sizeof(thread_N));
}

bool receiveTLV(SOCKET socket, uint8_t& type, std::vector<char>& buffer)
{
    uint32_t networkLength, length;

    if(recv(socket, (char*)&type, sizeof(type), 0) == SOCKET_ERROR)
    {
        return false;
    }

    if(recv(socket, (char*)&networkLength, sizeof(networkLength), 0) == SOCKET_ERROR)
    {
        return false;
    }

    length = ntohl(networkLength);
    buffer.resize(length);

    int received = 0;
    while(received < length)
    {
        int bytes = recv(socket, buffer.data() + received, length - received, 0);
        if(bytes == SOCKET_ERROR)
        {
            return false;
        }
        received += bytes;
    }

    return true;
}

int receiveRespond(SOCKET socket)
{
    uint8_t type;
    std::vector<char> buffer;

    if (!receiveTLV(socket, type, buffer))
    {
        std::cerr << "Failed to receive valid command.\n";
        return -1;
    }

    int networkCommand;
    memcpy(&networkCommand, buffer.data(), sizeof(int));
    return ntohl(networkCommand);
}

void receiveData(SOCKET socket, MatrixData& data)
{
    uint8_t type;
    std::vector<char> buffer;

    if (!receiveTLV(socket, type, buffer))
    {
        std::cerr << "Failed to receive TLV segment.\n";
        return;
    }

    int totalElements = buffer.size() / sizeof(int);
    data.matrixSize = static_cast<int>(sqrt(totalElements));
    data.matrix.resize(data.matrixSize, std::vector<int>(data.matrixSize));

    std::vector<int> matrixArray(totalElements);
    memcpy(matrixArray.data(), buffer.data(), totalElements * sizeof(int));

    int index = 0;
    for (int i = 0; i < data.matrixSize; ++i)
    {
        for (int j = 0; j < data.matrixSize; ++j)
        {
            data.matrix[i][j] = ntohl(matrixArray[index++]);
        }
    }

    std::cout << "Matrix data received from the server.\n";
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

    sendCommand(clientSocket, IS_BUSY);
    int message = receiveRespond(clientSocket);
    if (message == BUSY)
    {
        std::cerr << "Server is busy. Please try again later." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    sockaddr_in localAddr;
    int addrLen = sizeof(localAddr);
    getsockname(clientSocket, (sockaddr*)&localAddr, &addrLen);

    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &localAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
    int clientPort = ntohs(localAddr.sin_port);

    std::cout << "Client Address: " << clientIp << " | Port: " << clientPort << std::endl;

    int matrixSize, threadsCount;
    std::cout << "Enter matrix size: ";
    std::cin >> matrixSize;

    std::cout << "Enter number of threads: ";
    std::cin >> threadsCount;

    MatrixData data(matrixSize, threadsCount);
    // std::cout << "Initial matrix" << std::endl;
    // data.printMatrix();

    int row_test = data.matrixSize - 1, col_test = 0;
    int initial_value = data.matrix[row_test][col_test];
    // std::cout << "\nInitial value at [" << row_test << "][" << col_test << "]: " << initial_value << "\n";

    sendCommand(clientSocket, CONFIG);
    sendData(clientSocket, data);

    std::cout << "Matrix data sent to the server.\n";

    message = receiveRespond(clientSocket);
    if (message != CONFIG_OK)
    {
        std::cerr << "Config error." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    sendCommand(clientSocket, CALCULATE);
    message = receiveRespond(clientSocket);
    if(message != CALCULATION_STARTED)
    {
        std::cerr << "Calculation start error." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    while(true)
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        sendCommand(clientSocket, GET_RESULT);
        message = receiveRespond(clientSocket);
        if(message == CURRENT_PROGRESS)
        {
            int progressNet;
            recv(clientSocket, (char*)&progressNet, sizeof(progressNet), 0);
            int progress = ntohl(progressNet);
            std::cout << "Progress: " << progress << "% of the data processed\n";
        }
        else if(message == COMPLETED)
        {
            receiveData(clientSocket, data);

            std::cout << "All data has been processed. Final matrix received.\n";

            int new_value = data.matrix[row_test][col_test];
            // std::cout << "\nNew value at [" << row_test << "][" << col_test<< "]: " << new_value << "\n";
            if (new_value != initial_value)
            {
                std::cout << "Matrix was updated correctly.\n";
            }
            else
            {
                std::cout << "Matrix value did not change.\n";
            }
            // std::cout << "Received processed matrix:\n";
            // data.printMatrix();

            break;
        }
        else
        {
            std::cerr << "Error receiving result."<< std::endl;
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
