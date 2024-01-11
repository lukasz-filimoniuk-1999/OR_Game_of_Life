#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <iostream>
#include <map>

int rank,size;
char **Image;
const int imgWidth=200;
const int imgHeight=200;
const int square_size = 100;
const int stepLimit=1000;
int dividesVer = (imgHeight-2)/square_size;
int dividesHor = (imgWidth-2)/square_size;
char Row[imgWidth];
char OldRow[imgWidth];
char occ = 255;
char empty = 0;
char endChar = 100;

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

void WritePGM(const char *fname,int Width,int Height)
{
	// FILE *file=fopen(fname,"wt");
	// if (fname==NULL) return;
	// fprintf(file,"P5 %d %d 255\n",Width,Height);
	// for(int i=0;i<Height;i++)
	// 	fwrite(Image[i],sizeof(char),Width,file);
	// fclose(file);
}

void Master()
{
	int NextRow,ActiveSlaves;
	NextRow = 0;
	MPI_Status Status;
    char* emptyRow = new char[imgWidth];
	char* endRow = new char[imgWidth];

    for (int i=0; i<imgWidth; i++)
	{
        emptyRow[i] = empty;
		endRow[i] = endChar;
	}

    Image[61][61] = occ;
	Image[61][62] = occ;
	Image[61][63] = occ;
	Image[61][65] = occ;
	Image[62][61] = occ;
	Image[63][64] = occ;
	Image[63][65] = occ;
	Image[64][62] = occ;
	Image[64][63] = occ;
	Image[64][65] = occ;
	Image[65][61] = occ;
	Image[65][63] = occ;
	Image[65][65] = occ;


    WritePGM("init_file.pgm",imgWidth,imgHeight);
	
	ActiveSlaves=size-1;
    int Step = 0;
	for (int i = 0; i < dividesVer; i++)
		for (int j = 0; j < dividesHor; j++) {
			char* message = new char[(square_size+2)*(square_size+2)];
			int Slave = (i+j)%(size-1)+1;
			for (int ii = 0; ii < square_size+2; ii++)
				for (int jj = 0; jj < square_size+2; jj++){
						message[ii*(square_size+2)+jj] = Image[i * square_size + ii][j * square_size + jj];
					}
			MPI_Send(message, (square_size+2)*(square_size+2), MPI_CHAR,Slave,i*dividesHor+j,MPI_COMM_WORLD);
		}
	for (int step = 0; step < stepLimit; step++){
		for (int i = 0; i < dividesVer; i++)
			for (int j = 0; j < dividesHor; j++) {
				int Slave = (i+j)%(size-1)+1;
				char* message = new char[(square_size)*(square_size)];
				MPI_Recv(message, (square_size)*(square_size), MPI_CHAR,Slave,i*dividesHor+j,MPI_COMM_WORLD,&Status);
				for (int ii = 1; ii < square_size+1; ii++)
					for (int jj = 1; jj < square_size+1; jj++){
						Image[i * square_size + ii][j * square_size + jj] = message[(ii-1)*(square_size)+jj-1];
				}
			}
		char stepFileName[20];
		sprintf(stepFileName, "%dStep.pgm", step);
		WritePGM(stepFileName,imgWidth,imgHeight);
	}
	
}

void Slave()
{
    char *Row = new char[imgWidth];
    char *OldRow = new char[imgWidth];
    char *PrevRow = new char[imgWidth];
    char *NextRow = new char[imgWidth];
	MPI_Status Status;
    char* emptyRow = new char[imgWidth];

	for (int i = 0; i < dividesVer; i++)
		for (int j = 0; j < dividesHor; j++) {
			int Slave = (i+j)%(size-1)+1;
			if (Slave == rank) {
				char* message = new char[(square_size+2)*(square_size+2)];
				MPI_Recv(message, (square_size+2)*(square_size+2), MPI_CHAR,0,i*dividesHor+j,MPI_COMM_WORLD,&Status);
				for (int ii = 0; ii < square_size+2; ii++)
					for (int jj = 0; jj < square_size+2; jj++){
						Image[i * square_size + ii][j * square_size + jj] = message[ii*(square_size+2)+jj];
					}
			}
		}
	int PrevSlave = rank-1;
	if (rank == 1) PrevSlave = size-1;
	int NextSlave = rank+1;
	if (rank == size-1) NextSlave = 1;

	for(int step = 0; step < stepLimit; step++)
	{
		char **PrevImage=new char *[imgHeight];
		for(int i=0;i<imgHeight;i++)
		{
			PrevImage[i]=new char[imgWidth];
			for (int j=0; j<imgWidth;j++)
				PrevImage[i][j]=Image[i][j];
		}
		for (int i = 0; i < dividesVer; i++)
			for (int j = 0; j < dividesHor; j++) {
				int Slave = (i+j)%(size-1)+1;
					if (Slave == rank) {
						for (int ii = 1; ii < square_size+1; ii++)
							for (int jj = 1; jj < square_size+1; jj++){
								int X = i * square_size + ii;
								int Y = j * square_size + jj;
								int neighbors = 0;
								if (PrevImage[X-1][Y-1] == occ) neighbors++;
								if (PrevImage[X-1][Y] == occ) neighbors++;
								if (PrevImage[X-1][Y+1] == occ) neighbors++;
								if (PrevImage[X][Y-1] == occ) neighbors++;
								if (PrevImage[X][Y+1] == occ) neighbors++;
								if (PrevImage[X+1][Y-1] == occ) neighbors++;
								if (PrevImage[X+1][Y] == occ) neighbors++;
								if (PrevImage[X+1][Y+1] == occ) neighbors++;
								if (PrevImage[X][Y] == occ){
									if (neighbors >= 4) Image[X][Y] = empty;
									if (neighbors < 2) Image[X][Y] = empty;
								} else
									if (PrevImage[X][Y] != occ){
										if (neighbors == 3) Image[X][Y] = occ;
								}
								if (Image[X][Y] != occ)
									Image[X][Y] = rank*4;
							}
					}
			}
		for (int i = 0; i < dividesVer; i++)
			for (int j = 0; j < dividesHor; j++) {
				int Slave = (i+j)%(size-1)+1;
				if (Slave == rank) {
					MPI_Request req[2];
					MPI_Status stat[2];
					char* message = new char[(square_size)*(square_size)];
					for (int ii = 1; ii < square_size+1; ii++)
						for (int jj = 1; jj < square_size+1; jj++){
							message[(ii-1)*(square_size)+jj-1] = Image[i * square_size + ii][j * square_size + jj];
						}
					MPI_Send(message, (square_size)*(square_size), MPI_CHAR,0,i*dividesHor+j,MPI_COMM_WORLD);
					MPI_Isend(message, (square_size)*(square_size), MPI_CHAR,PrevSlave,i*dividesHor+j,MPI_COMM_WORLD,&req[0]);
					MPI_Isend(message, (square_size)*(square_size), MPI_CHAR,NextSlave,i*dividesHor+j,MPI_COMM_WORLD,&req[1]);
				}
				if (PrevSlave==Slave){
					char* message = new char[(square_size)*(square_size)];
					MPI_Recv(message,(square_size)*(square_size),MPI_CHAR,PrevSlave,i*dividesHor+j,MPI_COMM_WORLD, &Status);
					for (int ii = 1; ii < square_size+1; ii++)
						for (int jj = 1; jj < square_size+1; jj++){
							Image[i * square_size + ii][j * square_size + jj] = message[(ii-1)*(square_size)+jj-1];
						}
				}
				if (NextSlave==Slave){
					char* message = new char[(square_size)*(square_size)];
					MPI_Recv(message,(square_size)*(square_size),MPI_CHAR,NextSlave,i*dividesHor+j,MPI_COMM_WORLD, &Status);
					for (int ii = 1; ii < square_size+1; ii++)
						for (int jj = 1; jj < square_size+1; jj++){
							Image[i * square_size + ii][j * square_size + jj] = message[(ii-1)*(square_size)+jj-1];
						}
				}
			}
	}

}

int main(int argc,char *argv[])
{
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	if (size<2) {
		printf("At least 2 processes are needed\n");
		MPI_Finalize();
		exit(-1);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	AllocMem(imgWidth,imgHeight);
    for (int i=0; i<imgHeight; i++)
        for (int j=0; j<imgWidth; j++)
            Image[i][j] = empty;
	if (rank==0) { 
		double t1=MPI_Wtime();
		Master();
		double t2=MPI_Wtime();
		printf("Execution time: %5.3f msecs\n",(t2-t1)*1000.0);
		WritePGM("final.pgm",imgWidth,imgHeight);
		FreeMem(imgWidth,imgHeight);
	} else
		Slave();
	MPI_Finalize();
}