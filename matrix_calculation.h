#ifndef MATRIX_CALCULATION_H
#define MATRIX_CALCULATION_H
#include <vector>
#include <atomic>
#include <thread>

class MatrixCalculation
{
private:
    std::vector<std::vector<int>>& matrix;
    int thread_id = 0;
    int thread_N = 0;
    int progress;
    std::thread worker;
public:
    MatrixCalculation(std::vector<std::vector<int>>& matrix, int threadId, int threadNum);

    void startCalculation();
    void columnsDistribution();
    void calculatingProductOfColumn(const int col);
    int getProgress() const;
};

#endif //MATRIX_CALCULATION_H
