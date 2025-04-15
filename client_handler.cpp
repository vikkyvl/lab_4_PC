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
    std::cout << "Client disconnected" << std::endl;
    closesocket(clientSocket);
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

void ClientHandler::receiveData()
{
    int sizeReceived;
    int bytes = recv(clientSocket, (char*)&sizeReceived, sizeof(sizeReceived), 0);
    if (bytes == SOCKET_ERROR)
    {
        std::cerr << "Failed to receive matrix size." << std::endl;
        isSession = false;
        return;
    }
    int N = ntohl(sizeReceived);

    matrix.resize(N, std::vector<int>(N));

    int totalElements = N * N;
    std::vector<int> matrixArray(totalElements);

    int totalBytes = totalElements * sizeof(int);
    int receivedBytes = 0;

    while (receivedBytes < totalBytes)
    {
        int bytes = recv(clientSocket, (char*)matrixArray.data() + receivedBytes, totalBytes - receivedBytes, 0);
        if (bytes <= 0)
        {
            std::cerr << "Error receiving matrix data.\n";
            return;
        }
        receivedBytes += bytes;
    }

    int ij = 0;
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            matrix[i][j] = ntohl(matrixArray[ij++]);
        }
    }

    int threadsReceived;
    if (recv(clientSocket, (char*)&threadsReceived, sizeof(threadsReceived), 0) <= 0)
    {
        std::cerr << "Error receiving threads count.\n";
        isSession = false;
        return;
    }
    thread_N = ntohl(threadsReceived);

    std::cout << "Received matrix size: " << N << " and threads: " << thread_N << "\n";
}

void ClientHandler::sendRespond(ServerRespond response)
{
    int message = htonl(response);
    send(clientSocket, (char*)&message, sizeof(message), 0);
}

void ClientHandler::sendCalculatedMatrix()
{
    int matrixSize = matrix.size();
    int networkSize = htonl(matrixSize);

    send(clientSocket, (char*)&networkSize, sizeof(networkSize), 0);

    int totalElements = matrixSize * matrixSize;
    std::vector<int> matrixArray;
    matrixArray.reserve(totalElements);

    for (const auto& row : matrix)
    {
        for (int value : row)
        {
            matrixArray.push_back(htonl(value));
        }
    }

    int totalBytes = totalElements * sizeof(int);
    int sentBytes = 0;

    while (sentBytes < totalBytes)
    {
        int bytesNow = send(clientSocket, (char*)matrixArray.data() + sentBytes, totalBytes - sentBytes, 0);
        if (bytesNow == SOCKET_ERROR)
        {
            std::cerr << "Error sending calculated matrix.\n";
            return;
        }
        sentBytes += bytesNow;
    }

    std::cout << "Matrix sent successfully to client.\n";
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
    int totalColumns = matrix.size();
    return (total * 100) / totalColumns;
}