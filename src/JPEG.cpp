#include "JPEG.h"
#include "NxNDCT.h"
#include <cmath>
#include "JPEGBitStreamWriter.h"


#define DEBUG(x) do{ qDebug() << #x << " = " << x;}while(0)


const uint8_t QuantLuminance[8*8] =
    { 16, 11, 10, 16, 24, 40, 51, 61,
      12, 12, 14, 19, 26, 58, 60, 55,
      14, 13, 16, 24, 40, 57, 69, 56,
      14, 17, 22, 29, 51, 87, 80, 62,
      18, 22, 37, 56, 68,109,103, 77,
      24, 35, 55, 64, 81,104,113, 92,
      49, 64, 78, 87,103,121,120,101,
      72, 92, 95, 98,112,100,103, 99 };

const uint8_t QuantChrominance[8*8] =
    { 17, 18, 24, 47, 99, 99, 99, 99,
      18, 21, 26, 66, 99, 99, 99, 99,
      24, 26, 56, 99, 99, 99, 99, 99,
      47, 66, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99 };

uint8_t zigzag_shuffle[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

struct imageProperties{
    int width;
    int height;
    int16_t* coeffs;
};

short* DCTUandV(const char input[], int N, double* DCTKernel)
{
    double* temp = new double[N*N];
    double* DCTCoefficients = new double[N*N];
    short* output = new short[N*N];

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

    return output;
}


uint8_t quantQuality(uint8_t quant, uint8_t quality) {
    // Convert to an internal JPEG quality factor, formula taken from libjpeg
    int16_t q = quality < 50 ? 5000 / quality : 200 - quality * 2;
    return clamp((quant * q + 50) / 100, 1, 255);
}

template<typename T>
T* doZigZag(T block[], int N)
{
    T * out = new T[N * N];

    for (int i=0; i < 64; i++) {
        unsigned index = zigzag_shuffle[i];
        out[i] = block[index];
    }

    return out;
}

void writeBlock(int16_t block[], int YorUorV, JPEGBitStreamWriter* s)
{
    int N = 8;
    block = doZigZag(block, N);

    switch(YorUorV) {
      case 0:
        s->writeBlockY(block);
        break;
      case 1:
        s->writeBlockU(block);
        break;
      case 2:
        s->writeBlockV(block);
        break;
    }
}

void performJPEGEncoding(uchar Y_buff[], char U_buff[], char V_buff[], int xSize, int ySize, int quality)
{
    DEBUG(quality);
    int N = 8;

    auto s = new JPEGBitStreamWriter("example.jpg");

    char* Y_char = new char[xSize*ySize];
    for(int i = 0; i < ySize; i++)
    {
        for(int k = 0; k < xSize; k++)
            Y_char[i*xSize + k] = Y_buff[i*xSize + k] - 128.0;
    }

    int newxSizeY = xSize;
    int newySizeY = ySize;
    int newxSizeUV = xSize / 2;
    int newySizeUV = ySize / 2;

    char* Y_block1 = new char[N*N];
    char* Y_block2 = new char[N*N];
    char* Y_block3 = new char[N*N];
    char* Y_block4 = new char[N*N];
    char* U_block = new char[N*N];
    char* V_block = new char[N*N];

    char* outputY = new char[newxSizeY*newySizeY];
    char* outputV = new char[newxSizeUV*newySizeUV];
    char* outputU = new char[newxSizeUV*newySizeUV];

    if(xSize*ySize%16 != 0)
    {
        extendBorders(Y_char, xSize, ySize, N, &outputY, &newxSizeY, &newySizeY);
        extendBorders(U_buff, xSize, ySize, N, &outputU, &newxSizeUV, &newySizeUV);
        extendBorders(V_buff, xSize, ySize, N, &outputV, &newxSizeUV, &newySizeUV);
    }
    else
    {
        outputY = Y_char;
        outputV = V_buff;
        outputU = U_buff;
    }

    uint8_t* luminance = new uint8_t[N*N];
    uint8_t* chrominance = new uint8_t[N*N];

    for(int i = 0;i < N; i++)
    {
        for(int k = 0; k < N; k++)
        {
            luminance[i*N + k] = quantQuality(QuantLuminance[i*N + k], quality);
            chrominance[i*N + k] = quantQuality(QuantChrominance[i*N + k], quality);
        }
    }

    luminance = doZigZag(luminance, N);
    chrominance = doZigZag(chrominance, N);

    s->writeHeader();
    s->writeQuantizationTables(luminance, chrominance);
    s->writeImageInfo(newxSizeY, newySizeY);
    s->writeHuffmanTables();

    short* Ykoeficijenti_1 = new short[N*N];
    short* Ykoeficijenti_2 = new short[N*N];
    short* Ykoeficijenti_3 = new short[N*N];
    short* Ykoeficijenti_4 = new short[N*N];
    short* Ukoeficijenti = new short[N*N];
    short* Vkoeficijenti = new short[N*N];

    double* DCTKernel = new double[N*N];
    GenerateDCTmatrix(DCTKernel, N);

    for(int y = 0, yimage = 0; yimage < newySizeY/N; y++, yimage += 2)
    {
        for(int x = 0, ximage = 0; ximage < newxSizeY/N; x++, ximage += 2)
        {
            for(int i = 0; i < N; i++)
            {
                for(int k = 0; k < N; k++)
                {
                    // popunjavanje blokova
                    Y_block1[i*N+k] = outputY[(N*yimage+i)*(newxSizeY)+N*ximage+k];
                    Y_block2[i*N+k] = outputY[(N*yimage+i)*(newxSizeY)+N*(ximage+1)+k];
                    Y_block3[i*N+k] = outputY[(N*(yimage+1)+i)*(newxSizeY)+N*ximage+k];
                    Y_block4[i*N+k] = outputY[(N*(yimage+1)+i)*(newxSizeY)+N*(ximage+1)+k];
                    U_block[i*N+k] = outputU[(N*y+i)*(newxSizeUV)+N*x+k];
                    V_block[i*N+k] = outputV[(N*y+i)*(newxSizeUV)+N*x+k];
                }
            }

        // primena dct-a
        Ykoeficijenti_1 = DCTUandV(Y_block1, N, DCTKernel);
        Ykoeficijenti_2 = DCTUandV(Y_block2, N, DCTKernel);
        Ykoeficijenti_3 = DCTUandV(Y_block3, N, DCTKernel);
        Ykoeficijenti_4 = DCTUandV(Y_block4, N, DCTKernel);
        Ukoeficijenti = DCTUandV(U_block, N, DCTKernel);
        Vkoeficijenti = DCTUandV(V_block, N, DCTKernel);

        // kvantizacija
        for(int i = 0; i < N; i++)
        {
            for(int k = 0; k < N; k++)
            {
                Ykoeficijenti_1 [k*N + i] = round(Ykoeficijenti_1 [k*N + i] / luminance[k*N + i]);
                Ykoeficijenti_2 [k*N + i] = round(Ykoeficijenti_2 [k*N + i] / luminance[k*N + i]);
                Ykoeficijenti_3 [k*N + i] = round(Ykoeficijenti_3 [k*N + i] / luminance[k*N + i]);
                Ykoeficijenti_4 [k*N + i] = round(Ykoeficijenti_4 [k*N + i] / luminance[k*N + i]);
                Ukoeficijenti [k*N + i] = round(Ukoeficijenti [k*N + i] / chrominance[k*N + i]);
                Vkoeficijenti [k*N + i] = round(Vkoeficijenti [k*N + i] / chrominance[k*N + i]);
            }
        }

        // do zigzag and write to file
        writeBlock((int16_t*)Ykoeficijenti_1, 0, s);
        writeBlock((int16_t*)Ykoeficijenti_2, 0, s);
        writeBlock((int16_t*)Ykoeficijenti_3, 0, s);
        writeBlock((int16_t*)Ykoeficijenti_4, 0, s);
        writeBlock((int16_t*)Ukoeficijenti, 1, s);
        writeBlock((int16_t*)Vkoeficijenti, 2, s);

        }
    }

    s->finishStream();

    delete[] Y_block1;
    delete[] Y_block2;
    delete[] Y_block3;
    delete[] Y_block4;
    delete[] U_block;
    delete[] V_block;

    delete[] Ykoeficijenti_1;
    delete[] Ykoeficijenti_2;
    delete[] Ykoeficijenti_3;
    delete[] Ykoeficijenti_4;
    delete[] Ukoeficijenti;
    delete[] Vkoeficijenti;

    delete[] DCTKernel;
    delete[] luminance;
    delete[] chrominance;
    delete[] Y_char;
}

