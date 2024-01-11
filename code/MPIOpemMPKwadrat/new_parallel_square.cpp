#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <mpi.h>
#include <omp.h>


int ***Image;

void AllocateMemory(int n);
void InitImage(int n);
void FreeMemory(int n);
void WritePGM(const char *fname, int n, int index);
void GameOfLifeParralelSquares(int n, int stepLimit, int rank, int numOfProcesses);

int main(int argc, char *argv[]) {
    
    int imgSize = atoi(argv[1]) + 2; 
    int stepLimit = atoi(argv[2]);
    AllocateMemory(imgSize);
    InitImage(imgSize);


    MPI_Init(&argc, &argv);

    int rank, numOfProcesses;
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&numOfProcesses);

    double t1 = MPI_Wtime();

    GameOfLifeParralelSquares(imgSize, stepLimit, rank, numOfProcesses);

    double t2 = MPI_Wtime();

    if(rank == 0){
        printf("Execution time: %5.3f msecs\n", (t2 - t1) * 1000.0);
        WritePGM("final.pgm", imgSize, (stepLimit-1)%2);
    }

    MPI_Finalize();


    FreeMemory(imgSize);
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


void GameOfLifeParralelSquares(int n, int stepLimit, int rank, int numOfProcesses) {
    int squareSize = sqrt(numOfProcesses);

    if(rank >= squareSize * squareSize) return;
    
    int rankRow = rank / squareSize;
    int rankColumn = rank - rankRow * squareSize;

    int minRow = (n - 2) * rankRow / squareSize + 1;
    int maxRow = (n - 2) * (rankRow + 1) / squareSize;
    int minColumn = (n - 2) * rankColumn / squareSize + 1;
    int maxColumn = (n - 2) * (rankColumn + 1) / squareSize;

    for (int step = 1; step < stepLimit; ++step) {

        int actualIndex = step & 1;
        int prevIndex = 1 - actualIndex;

        #pragma omp parallel for
        for (int i = minRow; i <= maxRow; i++) {
            for (int j = minColumn; j <= maxColumn; ++j) {
                int neighbors = 0;
                if(Image[prevIndex][i-1][j-1]) neighbors++;
                if(Image[prevIndex][i-1][j]) neighbors++;
                if(Image[prevIndex][i-1][j+1]) neighbors++;
                if(Image[prevIndex][i][j-1]) neighbors++;
                if(Image[prevIndex][i][j+1]) neighbors++;
                if(Image[prevIndex][i+1][j-1])neighbors++;
                if(Image[prevIndex][i+1][j])neighbors++;
                if(Image[prevIndex][i+1][j+1])neighbors++;

                if(Image[prevIndex][i][j]){
                    if(neighbors > 3){
                        Image[actualIndex][i][j] = 0;
                    }
                    else if(neighbors < 2){
                        Image[actualIndex][i][j] = 0;
                    }
                    else{
                        Image[actualIndex][i][j] = 1;
                    }
                }
                else{
                    if(neighbors == 3){
                        Image[actualIndex][i][j] = 1;
                    }
                    else{
                        Image[actualIndex][i][j] = 0;
                    }
                }
            }
        }

        int rec, rec2, rec3, rec4;
        int send, send2, send3, send4;

        if(rankRow == 0){
            if(rankColumn == 0){
                MPI_Request reqtab[4];
                MPI_Status stattab[4];
                MPI_Isend(&send, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[0]);
                MPI_Isend(&send2, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[1]);
                MPI_Irecv(&rec, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[2]);
                MPI_Irecv(&rec2, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[3]);
                MPI_Waitall(4,reqtab,stattab);
            }
            else if(rankColumn == squareSize - 1){
                MPI_Request reqtab[4];
                MPI_Status stattab[4];
                MPI_Isend(&send, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[0]);
                MPI_Isend(&send2, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[1]);
                MPI_Irecv(&rec, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[2]);
                MPI_Irecv(&rec2, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[3]);
                MPI_Waitall(4,reqtab,stattab);
            }
            else{
                MPI_Request reqtab[6];
                MPI_Status stattab[6];
                MPI_Isend(&send, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[0]);
                MPI_Isend(&send2, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[1]);
                MPI_Isend(&send3, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[2]);
                MPI_Irecv(&rec, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[3]);
                MPI_Irecv(&rec2, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[4]);
                MPI_Irecv(&rec3, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[5]);
                MPI_Waitall(6,reqtab,stattab);
            }
        }
        else if(rankRow == squareSize - 1){
            if(rankColumn == 0){
                MPI_Request reqtab[4];
                MPI_Status stattab[4];
                MPI_Isend(&send, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[0]);
                MPI_Isend(&send2, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[1]);
                MPI_Irecv(&rec, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[2]);
                MPI_Irecv(&rec2, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[3]);
                MPI_Waitall(4,reqtab,stattab);
            }
            else if(rankColumn == squareSize - 1){
                MPI_Request reqtab[4];
                MPI_Status stattab[4];
                MPI_Isend(&send, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[0]);
                MPI_Isend(&send2, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[1]);
                MPI_Irecv(&rec, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[2]);
                MPI_Irecv(&rec2, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[3]);
                MPI_Waitall(4,reqtab,stattab);
            }
            else{
                MPI_Request reqtab[6];
                MPI_Status stattab[6];
                MPI_Isend(&send, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[0]);
                MPI_Isend(&send2, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[1]);
                MPI_Isend(&send3, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[2]);
                MPI_Irecv(&rec, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[3]);
                MPI_Irecv(&rec2, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[4]);
                MPI_Irecv(&rec3, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[5]);
                MPI_Waitall(6,reqtab,stattab);
            }
        }
        else{
            if(rankColumn == 0){
                MPI_Request reqtab[6];
                MPI_Status stattab[6];
                MPI_Isend(&send, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[0]);
                MPI_Isend(&send2, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[1]);
                MPI_Isend(&send3, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[2]);
                MPI_Irecv(&rec, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[3]);
                MPI_Irecv(&rec2, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[4]);
                MPI_Irecv(&rec3, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[5]);
                MPI_Waitall(6,reqtab,stattab);
            }
            else if(rankColumn == squareSize - 1){
                MPI_Request reqtab[6];
                MPI_Status stattab[6];
                MPI_Isend(&send, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[0]);
                MPI_Isend(&send2, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[1]);
                MPI_Isend(&send3, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[2]);
                MPI_Irecv(&rec, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[3]);
                MPI_Irecv(&rec2, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[4]);
                MPI_Irecv(&rec3, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[5]);
                MPI_Waitall(6,reqtab,stattab);
            }
            else{
                MPI_Request reqtab[8];
                MPI_Status stattab[8];
                MPI_Isend(&send, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[0]);
                MPI_Isend(&send2, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[1]);
                MPI_Isend(&send3, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[2]);
                MPI_Isend(&send4, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[3]);
                MPI_Irecv(&rec, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &reqtab[4]);
                MPI_Irecv(&rec2, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &reqtab[5]);
                MPI_Irecv(&rec3, 1, MPI_INT, rank-squareSize, 0, MPI_COMM_WORLD, &reqtab[6]);
                MPI_Irecv(&rec4, 1, MPI_INT, rank+squareSize, 0, MPI_COMM_WORLD, &reqtab[7]);
                MPI_Waitall(8,reqtab,stattab);
            }
        }

        // if(rank == 0){
        //     char stepFileName[20];
        //     sprintf(stepFileName, "%dstep.ppm", step);
        //     WritePGM(stepFileName, n, actualIndex);
        // }
    }
}
