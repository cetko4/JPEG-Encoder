#define _USE_MATH_DEFINES
#include <math.h>
#include "NxNDCT.h"

#define PI 3.14159265359

void GenerateDCTmatrix(double* DCTKernel, int order)
{
    int i, j;
    int N;
    N = order;
    double alpha;
    double denominator;
    for (j = 0; j <= N - 1; j++)
    {
        DCTKernel[0, j] = sqrt(1 / (double)N);
    }
    alpha = sqrt(2 / (double)N);
    denominator = (double)2 * N;
    for (j = 0; j <= N - 1; j++)
        for (i = 1; i <= N - 1; i++)
        {
            DCTKernel[i*N + j] = alpha * cos(((2 * j + 1) * i * PI) / denominator);
        }
}

void DCT(const char input[], short output[], int N, double* DCTKernel)
{
    double* temp = new double[N*N];
    double* DCTCoefficients = new double[N*N];

    double sum;
    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + DCTKernel[i*N+k] * (input[k*N+j]);
            }
            temp[i*N + j] = sum;
        }
    }

    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + temp[i*N+k] * DCTKernel[j*N+k];
            }
            DCTCoefficients[i*N+j] = sum;
        }
    }

    for(int i = 0; i < N*N; i++)
    {
        output[i] = floor(DCTCoefficients[i]+0.5);
    }

    delete[] temp;
    delete[] DCTCoefficients;

    return;
}

void IDCT(const short input[], uchar output[], int N, double* DCTKernel)
{
    /* TO DO */
    double* temp = new double[N*N];
        double* DCTCoefficients = new double[N*N];

        double sum;
        for (int i = 0; i <= N - 1; i++)
        {
            for (int j = 0; j <= N - 1; j++)
            {
                sum = 0;
                for (int k = 0; k <= N - 1; k++)
                {
                    sum = sum + DCTKernel[k*N+i] * input[k*N+j];
                }
                temp[i*N + j] = sum;
            }
        }

        for (int i = 0; i <= N - 1; i++)
        {
            for (int j = 0; j <= N - 1; j++)
            {
                sum = 0;
                for (int k = 0; k <= N - 1; k++)
                {
                    sum = sum + temp[i*N+k] * DCTKernel[k*N+j];
                }
                DCTCoefficients[i*N+j] = sum;
            }
        }

        for(int i = 0; i < N*N; i++)
        {
            output[i] = floor(DCTCoefficients[i]+0.5)+128;
        }

        delete[] temp;
        delete[] DCTCoefficients;

        return;
}

void extendBorders(char* input, int xSize, int ySize, int N, char** p_output, int* newXSize, int* newYSize)
{
    int deltaX = N-xSize%N;
    int deltaY = N-ySize%N;

    *newXSize = xSize+deltaX;
    *newYSize = ySize+deltaY;

    char* output = new char[(xSize+deltaX)*(ySize+deltaY)];

    for (int i=0; i<ySize; i++)
    {
        memcpy(&output[i*(xSize+deltaX)], &input[i*(xSize)], xSize);
        if(deltaX != 0)
        {
            memset(&output[i*(xSize+deltaX)+xSize], output[i*(xSize+deltaX)+xSize - 1], deltaX);
        }
    }

    for (int i=0; i<deltaY; i++)
    {
        memcpy(&output[(i + ySize)*(xSize+deltaX)], &output[(ySize)*(xSize+deltaX)], xSize+deltaX);
    }

    *p_output = output;
}

void cropImage(char* input, int xSize, int ySize, char* output, int newXSize, int newYSize)
{
    for (int i=0; i<newYSize; i++)
    {
        memcpy(&output[i*(newXSize)], &input[i*(xSize)], newXSize);
    }
}
