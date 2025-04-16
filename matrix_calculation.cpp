#include "matrix_calculation.h"

MatrixCalculation::MatrixCalculation(std::vector<std::vector<int> > &matrix, int threadId, int threadNum) : matrix(matrix), thread_id(threadId), thread_N(threadNum), progress(0) {}

void MatrixCalculation::startCalculation()
{
    worker = std::thread(&MatrixCalculation::columnsDistribution, this);
    worker.detach();
}

void MatrixCalculation::columnsDistribution()
{
    int step = thread_N;
    for(int col = thread_id; col < matrix.size(); col+=step)
    {
        calculatingProductOfColumn(col);
    }
}

void MatrixCalculation::calculatingProductOfColumn(const int col)
{
    int product = 1;
    const int N = matrix.size();
    for(int row = 0; row < N; row++)
    {
        product *= matrix[row][col];
    }

    matrix[N - 1 - col][col] = product;

    progress++;
}

int MatrixCalculation::getProgress() const
{
    return progress;
}
