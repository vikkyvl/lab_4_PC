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
    std::vector<std::vector<int>> matrix;
    ServerState serverState;
    int thread_N;
    bool isSession;
    std::vector<MatrixCalculation> threadsMatrixCalculation;

public:
    explicit ClientHandler(SOCKET clientSocket);
    void operator()();

    void handleConfig();
    void handleCalculate();
    void handleGetResult();

    void receiveData();
    int receiveCommand();

    void sendCalculatedMatrix();
    void sendRespond(ServerRespond response);

    void startCalculation();
    int getCalculationProgress();
};

#endif //CLIENT_HANDLER_H
