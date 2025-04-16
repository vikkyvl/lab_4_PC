#include "client_handler.h"
#include <math.h>
#include <iostream>

ClientHandler::ClientHandler(SOCKET socket, std::string ip, int port): clientSocket(socket), clientIp(std::move(ip)), clientPort(port), serverState(ServerState::INITIAL), isSession(true) {}

extern std::atomic<int> activeClients;

void ClientHandler::operator()()
{
    while (isSession)
    {
        int command = receiveCommand();

        if (command == IS_BUSY)
        {
            sendRespond(ACTIVE);
        }
        else if (command == CONFIG)
        {
            handleConfig();
        }
        else if (command == CALCULATE)
        {
            handleCalculate();
        }
        else if (command == GET_RESULT)
        {
            handleGetResult();
        }
        else
        {
            std::cout << "Command not recognized" << std::endl;
            sendRespond(FAILED);
        }
    }
    std::cout << "Client disconnected: " << clientIp << " | " << clientPort << std::endl;
    closesocket(clientSocket);

    activeClients.fetch_sub(1);
}

void ClientHandler::handleConfig()
{
    if(serverState == ServerState::INITIAL)
    {
        serverState = ServerState::CONFIGURED;
        receiveData();
        //isSession = false;
        sendRespond(CONFIG_OK);
    }
    else
    {
        sendRespond(FAILED);
    }
}

void ClientHandler::handleCalculate()
{
    if(serverState == ServerState::CONFIGURED)
    {
        startCalculation();
        serverState = ServerState::CALCULATING;
        sendRespond(CALCULATION_STARTED);
        // isSession = false;
    }
    else
    {
        sendRespond(FAILED);
    }
}

void ClientHandler::handleGetResult()
{
    if(serverState == ServerState::CALCULATING)
    {
        int progress = getCalculationProgress();
        if(progress < 100)
        {
            sendRespond(CURRENT_PROGRESS);
            int currentProgress = htonl(progress);
            send(clientSocket, (char*)&currentProgress, sizeof(currentProgress), 0);
            std::cout << "Progress sent to " << clientIp << ":" << clientPort << " - " << progress << "%" << std::endl;
        }
        else
        {
            isSession = false;
            sendRespond(COMPLETED);
            sendCalculatedMatrix();
        }
    }
    else
    {
        sendRespond(FAILED);
    }
}

bool ClientHandler::receiveTLV(uint8_t& type, std::vector<char>& buffer) const
{
    uint32_t networkLength, length;

    if(recv(clientSocket, (char*)&type, sizeof(type), 0) == SOCKET_ERROR)
    {
        return false;
    }

    if(recv(clientSocket, (char*)&networkLength, sizeof(networkLength), 0) == SOCKET_ERROR)
    {
        return false;
    }

    length = ntohl(networkLength);
    buffer.resize(length);

    int received = 0;
    while(received < length)
    {
        int bytes = recv(clientSocket, buffer.data() + received, length - received, 0);
        if(bytes == SOCKET_ERROR)
        {
            return false;
        }
        received += bytes;
    }

    return true;
}

int ClientHandler::receiveCommand()
{
    uint8_t type;
    std::vector<char> buffer;

    if (!receiveTLV(type, buffer))
    {
        std::cerr << "Failed to receive valid command.\n";
        isSession = false;
        return -1;
    }

    int netCommand;
    memcpy(&netCommand, buffer.data(), sizeof(int));
    return ntohl(netCommand);
}

void ClientHandler::receiveData()
{
    uint8_t type;
    std::vector<char> buffer;
    std::vector<int> matrixArray;

    for(int i = 0; i < 2; i++)
    {
        if (!receiveTLV(type, buffer))
        {
            std::cerr << "Failed to receive TLV segment.\n";
            isSession = false;
            return;
        }

        if(type == MATRIX)
        {
            int totalElements = buffer.size() / sizeof(int);
            N = static_cast<int>(sqrt(totalElements));

            matrixArray.resize(totalElements);
            memcpy(matrixArray.data(), buffer.data(), buffer.size());
        }
        else if(type == THREADS)
        {
            int threadsReceived;
            memcpy(&threadsReceived, buffer.data(), sizeof(int));
            thread_N = ntohl(threadsReceived);
        }
    }

    matrix.resize(N, std::vector<int>(N));
    int ij = 0;
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            matrix[i][j] = ntohl(matrixArray[ij++]);
        }
    }

    std::cout << "Received matrix size: " << N << " and threads: " << thread_N << " from " << clientIp << " | " << clientPort << std::endl;
}

void ClientHandler::sendTLV(uint8_t type, const void* data, uint32_t length) const
{
    send(clientSocket, (char*)&type, sizeof(type), 0);

    uint32_t netLength = htonl(length);
    send(clientSocket, (char*)&netLength, sizeof(netLength), 0);

    send(clientSocket, (char*)data, length, 0);
}

void ClientHandler::sendRespond(ServerRespond response)
{
    int message = htonl(response);
    sendTLV(COMMAND, &message, sizeof(message));
}

void ClientHandler::sendCalculatedMatrix()
{
    int totalElements = N * N;

    for (const auto& row : matrix)
    {
        for (int value : row)
        {
            matrixArray.push_back(htonl(value));
        }
    }

    sendTLV(MATRIX, matrixArray.data(), totalElements * sizeof(int));

    std::cout << "Matrix sent successfully to client " << clientIp << " | " << clientPort << std::endl;;
}

void ClientHandler::startCalculation()
{
    for(int i = 0; i < thread_N; i++)
    {
        threadsMatrixCalculation.emplace_back(matrix, i, thread_N);
    }

    for(auto& threadMatrixCalculation : threadsMatrixCalculation)
    {
        threadMatrixCalculation.startCalculation();
    }
}

int ClientHandler::getCalculationProgress()
{
    int total = 0;
    for(auto& threadMatrixCalculation : threadsMatrixCalculation)
    {
        total += threadMatrixCalculation.getProgress();
    }
    int totalColumns = N;
    return (total * 100) / totalColumns;
}