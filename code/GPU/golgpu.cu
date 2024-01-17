#include <cstdio>
#include <cstdlib>
#include <chrono>

int ***Image;

void AllocateMemory(int n);
void FreeMemory(int n);
void InitImage(int n);
void WritePGM(const char *fname, int n, int index);
void GameOfLifeGPU(int n, int stepLimit);

int main(int argc, char *argv[]) {

    int imgSize = atoi(argv[1]) + 2; // +2 dla uproszeczenia warunkow
    int stepLimit = atoi(argv[2]);

    AllocateMemory(imgSize);
    InitImage(imgSize);
    
    auto start_time = std::chrono::high_resolution_clock::now();

    GameOfLifeGPU(imgSize, stepLimit);

    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    printf("Execution time: %d msecs\n", duration.count());

    WritePGM("final.pgm", imgSize, (stepLimit-1)%2);

    FreeMemory(imgSize);

    //MPI_Finalize();

    return 0;
}

void AllocateMemory(int n) {
    Image = new int **[2];
    for (int i = 0; i < 2; ++i) {
        Image[i] = new int *[n];
        for (int j = 0; j < n; ++j) {
            Image[i][j] = new int[n];
        }
    }
}

void InitImage(int n){
    for (int i = 0; i < n; i++){
        for (int j = 0; j < n; j++){
            Image[0][i][j] = 0;
        }
    }

    Image[0][1][7] = 1;
    Image[0][2][1] = 1;
    Image[0][2][2] = 1;
    Image[0][3][2] = 1;
    Image[0][3][6] = 1;
    Image[0][3][7] = 1;
    Image[0][3][8] = 1;
}

void FreeMemory(int n) {
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < n; ++j) {
            delete[] Image[i][j];
        }
        delete[] Image[i];
    }
    delete[] Image;
}


void WritePGM(const char *fname, int n, int index) {
    FILE *file = fopen(fname, "wt");
    if (!file) {
        perror("Error opening file");
        return;
    }

    fprintf(file, "P3\n%d %d\n255\n", n, n);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (Image[index][i][j] == 1) {
                fprintf(file, "255 0 0 ");
            } else {
                fprintf(file, "255 255 255 ");
            }
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

__global__ void GOLKernel(int* d_In, int* d_Out, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = i * n + j;

    if (i > 0 && i < n - 1 && j > 0 && j < n - 1) {
        int neighbors = 0;

        int i_minus_1 = (i - 1) * n;
        int i_plus_1 = (i + 1) * n;

        if (d_In[i_minus_1 + j - 1]) neighbors++;
        if (d_In[i_minus_1 + j]) neighbors++;
        if (d_In[i_minus_1 + j + 1]) neighbors++;
        if (d_In[idx - 1]) neighbors++;
        if (d_In[idx + 1]) neighbors++;
        if (d_In[i_plus_1 + j - 1]) neighbors++;
        if (d_In[i_plus_1 + j]) neighbors++;
        if (d_In[i_plus_1 + j + 1]) neighbors++;

        if (d_In[idx]) {
            if (neighbors > 3) {
                d_Out[idx] = 0;
            } else if (neighbors < 2) {
                d_Out[idx] = 0;
            } else {
                d_Out[idx] = 1;
            }
        } else {
            if (neighbors == 3) {
                d_Out[idx] = 1;
            } else {
                d_Out[idx] = 0;
            }
        }
    }
}

void GameOfLifeGPU(int n, int stepLimit) {

    int *d_Image, *d_outputImage;
    cudaMalloc((void **)&d_Image, n * n * sizeof(int *));
    cudaMalloc((void **)&d_outputImage, n * n * sizeof(int *));

    for (int step = 1; step < stepLimit; ++step) {

        int actualIndex = step & 1;
        int prevIndex = 1 - actualIndex;

        int* h_Image = new int[n * n];
        int* h_outputImage = new int[n * n];

        for (int i = 0; i < n; i++) {
            cudaMemcpy(h_Image + i * n, Image[prevIndex][i], n * sizeof(int), cudaMemcpyHostToHost);
        }

        cudaMemcpy(d_Image, h_Image, n * n * sizeof(int), cudaMemcpyHostToDevice);

        delete[] h_Image;

        dim3 blockSize(16, 16);
        dim3 gridSize((n + blockSize.x - 1) / blockSize.x, (n + blockSize.y - 1) / blockSize.y);

        GOLKernel <<<gridSize, blockSize>>> (d_Image, d_outputImage, n);

        cudaMemcpy(h_outputImage, d_outputImage, n * n * sizeof(int), cudaMemcpyDeviceToHost);

        for (int i = 0; i < n; ++i) {
            cudaMemcpy(Image[actualIndex][i], h_outputImage + i * n, n * sizeof(int), cudaMemcpyHostToHost);
        }
        
        delete[] h_outputImage;
    }

    cudaFree(d_Image);
    cudaFree(d_outputImage);

    cudaDeviceSynchronize();
}
