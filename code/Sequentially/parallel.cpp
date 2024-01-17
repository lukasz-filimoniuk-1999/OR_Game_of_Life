#include <cstdio>
#include <cstdlib>
#include <mpi.h>


int ***Image;

void AllocateMemory(int n);
void FreeMemory(int n);
void InitImage(int n);
void WritePGM(const char *fname, int n, int index);
void GameOfLifeCPU(int n, int stepLimit);

int main(int argc, char *argv[]) {
    
    MPI_Init(&argc, &argv);

    int imgSize = atoi(argv[1]) + 2; // +2 dla uproszeczenia warunkow
    int stepLimit = atoi(argv[2]);

    AllocateMemory(imgSize);
    InitImage(imgSize);

    double t1 = MPI_Wtime();

    GameOfLifeCPU(imgSize, stepLimit);

    double t2 = MPI_Wtime();

    printf("Execution time: %5.3f msecs\n", (t2 - t1) * 1000.0);

    WritePGM("final.pgm", imgSize, (stepLimit-1)%2);

    FreeMemory(imgSize);

    MPI_Finalize();

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

void GameOfLifeCPU(int n, int stepLimit) {

    for (int step = 1; step < stepLimit; ++step) {

        int actualIndex = step & 1;
        int prevIndex = 1 - actualIndex;

        for (int i = 1; i < n - 1; i++) {
            for (int j = 1; j < n - 1; ++j) {
                int neighbors = 0;
                if(Image[prevIndex][i-1][j-1]) neighbors++;
                if(Image[prevIndex][i-1][j]) neighbors++;
                if(Image[prevIndex][i-1][j+1]) neighbors++;
                if(Image[prevIndex][i][j-1]) neighbors++;
                if(Image[prevIndex][i][j+1]) neighbors++;
                if(Image[prevIndex][i+1][j-1])neighbors++;
                if(Image[prevIndex][i+1][j])neighbors++;
                if(Image[prevIndex][i+1][j+1])neighbors++;

                if(Image[prevIndex][i][j]) {
                    if(neighbors > 3){
                        Image[actualIndex][i][j] = 0;
                    }
                    else if(neighbors < 2) {
                        Image[actualIndex][i][j] = 0;
                    }
                    else{
                        Image[actualIndex][i][j] = 1;
                    }
                }
                else {
                    if(neighbors == 3) {
                        Image[actualIndex][i][j] = 1;
                    }
                    else {
                        Image[actualIndex][i][j] = 0;
                    }
                }
            }
        }

        // char stepFileName[20];
        // sprintf(stepFileName, "%dstep.ppm", step);
        // WritePGM(stepFileName, n, actualIndex);
    }
}
