#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <winsock2.h>
#include <vector>
#include "commands.h"
#include "matrix_calculation.h"

class ClientHandler
{
private:
    SOCKET clientSocket;
    std::string clientIp;
    int clientPort;

    std::vector<int> matrixArray;
    std::vector<std::vector<int>> matrix;
    std::vector<MatrixCalculation> threadsMatrixCalculation;

    int N;
    int thread_N;
    bool isSession;
    ServerState serverState;

public:
    explicit ClientHandler(SOCKET socket, std::string clientIp, int clientPort);
    void operator()();

    void handleConfig();
    void handleCalculate();
    void handleGetResult();

    bool receiveTLV(uint8_t& type, std::vector<char>& buffer) const;
    int receiveCommand();
    void receiveData();

    void sendTLV(uint8_t type, const void* data, uint32_t length) const;
    void sendRespond(ServerRespond response);
    void sendCalculatedMatrix();

    void startCalculation();
    int getCalculationProgress();
};

#endif //CLIENT_HANDLER_H
