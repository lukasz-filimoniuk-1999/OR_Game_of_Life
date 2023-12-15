#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int rank,size;
char **Image;
const int imgWidth=100;
const int imgHeight=100;
const int stepLimit=100;

void AllocMem(int Width,int Height) 
{
	Image=new char *[Height];
	for(int i=0;i<Height;i++)
		Image[i]=new char[Width];
}

void FreeMem(int Width,int Height) 
{
	for(int i=0;i<Height;i++)
		delete []  Image[i];
	delete [] Image;
}

void WritePGM(const char *fname, int Width, int Height)
{
    FILE *file = fopen(fname, "wt");
    if (file == NULL)
    {
        printf("Error opening file: %s\n", fname);
        return;
    }

    fprintf(file, "P3\n%d %d\n255\n", Width, Height);

    for (int i = 0; i < Height; i++)
    {
        for (int j = 0; j < Width; j++)
        {
            if (Image[i][j] == '#')
            {
                // Use color for "#"
                fprintf(file, "255 0 0 "); // Red color (you can modify these values)
            }
            else
            {
                // Use color for " "
                fprintf(file, "255 255 255 "); // White color (you can modify these values)
            }
        }
        fprintf(file, "\n");
    }

    fclose(file);
}



void GameOfLifeCPU(){
    for (int i=0; i<imgHeight; i++)
        for (int j=0; j<imgWidth; j++)
            Image[i][j] = ' ';

    Image[0][6] = '#';
    Image[1][0] = '#';
    Image[1][1] = '#';
    Image[2][1] = '#';
    Image[2][5] = '#';
    Image[2][6] = '#';
    Image[2][7] = '#';

    char* PrevRow = new char[imgWidth];
    char* OldRow = new char[imgWidth];
    char* NextRow = new char[imgWidth];

    int step = 0;

    while (step < stepLimit){
        ++step;
        for (int j=0; j<imgWidth; j++){
            PrevRow[j] = ' ';
            OldRow[j] = Image[0][j];
        }
        for(int i=0; i<imgHeight; i++){
            // Aktualizacja aktualnego i nastepnego wiersza
            for(int j=0; j<imgWidth; ++j){
                OldRow[j] = Image[i][j];
            }
            if(i < imgHeight - 1){
                for(int j=0; j<imgWidth; ++j){
                    NextRow[j] = Image[i+1][j];
                }
            }
            else{
                for(int j=0; j<imgWidth; ++j){
                    NextRow[j] = ' ';
                }
            }

            

            for(int j = 0; j<imgWidth; ++j){
                // Obliczenie liczby sasiadow
                int neighbors = 0;
                if(j > 0){
                    if(PrevRow[j-1] == '#') neighbors++;
                    if(OldRow[j-1] == '#') neighbors++;
                    if(NextRow[j-1] == '#') neighbors++;
                }
                if(j < imgWidth-1){
                    if(PrevRow[j+1] == '#') neighbors++;
                    if(OldRow[j+1] == '#') neighbors++;
                    if(NextRow[j+1] == '#') neighbors++;
                }
                if (PrevRow[j] == '#') neighbors++;
                if (NextRow[j] == '#') neighbors++;
                // Warunki przezycia komorki
                if (OldRow[j] == '#'){
                    if (neighbors > 3) Image[i][j] = ' ';
                    else if (neighbors < 2) Image[i][j] = ' ';
                    else Image[i][j] = '#';
                }
                else if (OldRow[j] == ' '){
                    if (neighbors == 3) Image[i][j] = '#';
                    else Image[i][j] = ' ';
                }
            }
            // Aktualizacja poprzedniego wiersza
            for(int j=0; j<imgWidth; ++j){
                PrevRow[j] = OldRow[j];
            }
        }
        char stepFileName[20];
        sprintf(stepFileName, "%dstep.pgm", step);
        WritePGM(stepFileName, imgWidth, imgHeight);
    }
    delete [] PrevRow;
    delete [] OldRow;
    delete [] NextRow;
}

int main(int argc,char *argv[]){
	MPI_Init(&argc, &argv);
    AllocMem(imgWidth,imgHeight);
	double t1=MPI_Wtime();
	GameOfLifeCPU();
	double t2=MPI_Wtime();
	printf("Execution time: %5.3f msecs\n",(t2-t1)*1000.0);
	WritePGM("final.pgm",imgWidth,imgHeight);
	FreeMem(imgWidth,imgHeight);
    MPI_Finalize();
}
