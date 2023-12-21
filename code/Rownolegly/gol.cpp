#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <iostream>
#include <map>

int rank,size;
char **Image;
const int imgWidth=15;
const int imgHeight=15;
const int stepLimit=100;
char Row[imgWidth];
char OldRow[imgWidth];
char occ = 255;
char empty = 0;
char endChar = 100;

void AllocMem(int Width,int Height) 
{
	//printf("AllocMem");
	Image=new char *[Height];
	for(int i=0;i<Height;i++)
		Image[i]=new char[Width];
}

void FreeMem(int Width,int Height) 
{
	//printf("FreeMem");
	for(int i=0;i<Height;i++)
		delete []  Image[i];
	delete [] Image;
}

void WritePGM(const char *fname,int Width,int Height)
{
	//printf("WritePGM");
	FILE *file=fopen(fname,"wt");
	if (fname==NULL) return;
	fprintf(file,"P5 %d %d 255\n",Width,Height);
	for(int i=0;i<Height;i++)
		fwrite(Image[i],sizeof(char),Width,file);
	fclose(file);
}

void Master()
{
	//printf("Master");
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

    Image[0][6] = occ;
    Image[1][0] = occ;
    Image[1][1] = occ;
    Image[2][1] = occ;
    Image[2][5] = occ;
    Image[2][6] = occ;
    Image[2][7] = occ;
    WritePGM("init_file.pgm",imgWidth,imgHeight);
	
	ActiveSlaves=size-1;
    int Step = 0;
    char* message = new char[imgWidth];
	do {
		//printf("\nStep %d/%d\n",Step,stepLimit);
		int Slave=(Step+NextRow)%(size-1)+1;
		//printf("\nRow %d/%d\n",NextRow,imgHeight);
		//printf("Next Row %d\n", NextRow);
			for (int i=0; i<imgWidth; i++)
				message[i] = Image[NextRow][i];
            MPI_Send(message, imgWidth, MPI_CHAR,Slave,NextRow,MPI_COMM_WORLD);
			//std::cout << NextRow << message << std::endl;
			//std::cout << "LINES SEND\n" << message[0] << '\n' << message[1] << '\n' << '\n' << message[2] << '\n';
			//MPI_Recv(Row,imgWidth,MPI_CHAR,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&Status);
			NextRow++;
			//int ReceivedRow=Status.MPI_TAG;
			//std::cout << "Row=" << Row << '\n';
			//Image[ReceivedRow] = Row;
			//memcpy(Image[ReceivedRow],Row,sizeof(char)*imgWidth);
		}
		// if (NextRow<imgHeight) {
        //     if (NextRow == 0) for (int i=0; i<imgWidth; i++) message[0][i] = emptyRow[i];
        //     else {for (int i=0; i<imgWidth; i++) {message[0][i] = OldRow[i];
        //         OldRow[i] = Image[NextRow][i];}}
		// 	//std::cout << "Image[NextRow]=" << Image[NextRow] << '\n';
		// 	for (int i=0; i<imgWidth; i++)
		// 		message[1][i] = Image[NextRow][i];
        //     if (NextRow <= imgHeight-2) 	
		// 		for (int i=0; i<imgWidth; i++)
		// 			message[2][i] = Image[NextRow+1][i];
        //     else {
		// 		std::cout << "EMPTY ROW COPY" << std::endl;
		// 		for (int i=0; i<imgWidth; i++) message[2][i] = emptyRow[i];
        //     }
        //     MPI_Send(message[0], imgWidth, MPI_CHAR,Slave,NextRow,MPI_COMM_WORLD);
        //     MPI_Send(message[1], imgWidth, MPI_CHAR,Slave,NextRow,MPI_COMM_WORLD);
        //     MPI_Send(message[2], imgWidth, MPI_CHAR,Slave,NextRow,MPI_COMM_WORLD);
		// 	//std::cout << "LINES SEND\n" << message[0] << '\n' << message[1] << '\n' << '\n' << message[2] << '\n';
		// 	ActiveSlaves++;
		// 	MPI_Recv(Row,imgWidth,MPI_CHAR,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&Status);
		// 	NextRow++;
		// 	ActiveSlaves--;
		// 	int ReceivedRow=Status.MPI_TAG;
		// 	//std::cout << "Row=" << Row << '\n';
		// 	//Image[ReceivedRow] = Row;
		// 	memcpy(Image[ReceivedRow],Row,sizeof(char)*imgWidth);
		// } else {
        //     NextRow=0;
        //     Step++;
		// 	char stepFileName[20];
		// 	sprintf(stepFileName, "%dstep.pgm",Step);
        //     WritePGM(stepFileName,imgWidth,imgHeight);
        //     if (Step > stepLimit) {
		// 	    char* minusOne=new char[imgWidth];
        //         minusOne[0] = 'C';
		// 	    MPI_Send(minusOne,imgWidth,MPI_CHAR,Slave,NextRow,MPI_COMM_WORLD);
        //     } else { 
		// 	    ActiveSlaves++;
        //     }
		// }
		//printf("\nActiveSlaves = %d\n", ActiveSlaves);
	while (NextRow<imgHeight);
	for (int i = 0; i < size; i++)
        MPI_Send(endRow, imgWidth, MPI_CHAR,i,imgHeight+1,MPI_COMM_WORLD);
	for (int step = 0; step < stepLimit; step++) {
		for (int rowNum = 0; rowNum < imgHeight; rowNum++) {
			//printf("Recieving line %d\n", rowNum);
			MPI_Recv(Image[rowNum],imgWidth,MPI_CHAR,MPI_ANY_SOURCE,rowNum,MPI_COMM_WORLD,&Status);
			//std::cout << Image[rowNum] << std::endl;
		}
		char stepFileName[20];
		sprintf(stepFileName, "%dstep.pgm",step);
        WritePGM(stepFileName,imgWidth,imgHeight);
	}
}

void Slave()
{
	//printf("Slave %d, ",rank);
	std::map<int, char*> mp;
    char *Row = new char[imgWidth];
    char *OldRow = new char[imgWidth];
    char *PrevRow = new char[imgWidth];
    char *NextRow = new char[imgWidth];
	MPI_Status Status;
    char* emptyRow = new char[imgWidth];
	while(1) {
		MPI_Recv(Row,imgWidth,MPI_CHAR,0,MPI_ANY_TAG,MPI_COMM_WORLD,&Status);
		//std::cout << Status.MPI_TAG << Row << std::endl;
		//std::cout << "LINES RECEIVED\n" << message[0] << '\n' << message[1] << '\n' << '\n' << message[2] << '\n';
		if (Row[1] == endChar) break;
		int RowNum = Status.MPI_TAG;
		mp[RowNum] = new char[imgWidth];
		for (int i = 0; i < imgWidth; i++)
		{
			mp[RowNum][i] = Row[i];
		}
		Image[RowNum] = Row;
		// for (int i = 0; i < imgWidth; i++){
        //     OldRow[i]=Row[i];
        //     int neighbors = 0;
        //     if (i > 0) {
        //         if (PrevRow[i-1] == occ) neighbors++;
        //         if (OldRow[i-1] == occ) neighbors++;
        //         if (NextRow[i-1] == occ) neighbors++;
        //     }
        //     if (i < imgWidth-1) {
        //         if (PrevRow[i+1] == occ) neighbors++;
        //         if (Row[i+1] == occ) neighbors++;
        //         if (NextRow[i+1] == occ) neighbors++;
        //     }
        //     if (PrevRow[i] == occ) neighbors++;
        //     if (NextRow[i] == occ) neighbors++;
        //     if (Row[i] == occ){
        //         if (neighbors >= 4) Row[i] = empty;
        //         if (neighbors < 2) Row[i] = empty;
        //     } else
        //     if (Row[i] == empty){
        //         if (neighbors == 3) Row[i] = occ;
        //     }
        // }
		// MPI_Send(Row,imgWidth,MPI_CHAR,0,RowNum,MPI_COMM_WORLD);
		//std::cout << "LINE SEND=" << Row << '\n';
	}
	for (int step = 0; step < stepLimit; step++){
		std::map<int, char*>::iterator it = mp.begin();
		int PrevSlave = rank-1;
		if (rank == 1) PrevSlave = size-1;
		int NextSlave = rank+1;
		if (rank == size-1) NextSlave = 1;
		MPI_Request reqs[3];
		MPI_Status stats[3];
		while (it != mp.end()) {
			int RowNum = it->first;
			Row = new char[imgWidth];
			for (int i = 0; i < imgWidth; i++)
			{
				Row[i] = it->second[i];
			}
			if (RowNum>0) {
				MPI_Irecv(Image[RowNum-1],imgWidth,MPI_CHAR,PrevSlave,RowNum-1,MPI_COMM_WORLD,&reqs[0]);
			}
			MPI_Isend(Row,imgWidth,MPI_CHAR,NextSlave,RowNum,MPI_COMM_WORLD, &reqs[1]);
			if (RowNum<imgHeight-1) {
				MPI_Irecv(Image[RowNum+1],imgWidth,MPI_CHAR,NextSlave,RowNum+1,MPI_COMM_WORLD,&reqs[0]);
			}
			MPI_Isend(Row,imgWidth,MPI_CHAR,PrevSlave,RowNum,MPI_COMM_WORLD, &reqs[2]);
			++it;
			MPI_Waitall(3, reqs, stats);
		}
		// if (rank == 1) PrevRow = emptyRow;
		// 	else {
		// 		MPI_Recv(PrewRow,imgWidth,MPI_CHAR,rank-1,MPI_ANY_TAG,MPI_COMM_WORLD,&Status);
		// 	}
		// if (rank == size-1) 
		it = mp.begin();
		while (it != mp.end()) {
			int RowNum = it->first;
			Row = new char[imgWidth];
			for (int i = 0; i < imgWidth; i++)
			{
				Row[i] = it->second[i];
			}
			if (RowNum>0) PrevRow = Image[RowNum-1]; else PrevRow = emptyRow;
			if (RowNum<imgHeight-1) NextRow = Image[RowNum+1]; else NextRow = emptyRow;
			for (int i = 0; i < imgWidth; i++){
				OldRow[i]=Row[i];
				int neighbors = 0;
				if (i > 0) {
					if (PrevRow[i-1] == occ) neighbors++;
					if (OldRow[i-1] == occ) neighbors++;
					if (NextRow[i-1] == occ) neighbors++;
				}
				if (i < imgWidth-1) {
					if (PrevRow[i+1] == occ) neighbors++;
					if (Row[i+1] == occ) neighbors++;
					if (NextRow[i+1] == occ) neighbors++;
				}
				if (PrevRow[i] == occ) neighbors++;
				if (NextRow[i] == occ) neighbors++;
				if (Row[i] == occ){
					if (neighbors >= 4) Row[i] = empty;
					if (neighbors < 2) Row[i] = empty;
				} else
				if (Row[i] != occ){
					if (neighbors == 3) Row[i] = occ;
				}
				if (Row[i] != occ){
					Row[i] = 4*rank;
				}
			}
			mp[RowNum] = Row;
			//printf("Sending line %d\n", RowNum);
			//std::cout << Row << std::endl;
			MPI_Send(Row,imgWidth,MPI_CHAR,0,RowNum,MPI_COMM_WORLD);
			++it;
		}
	}
}

int main(int argc,char *argv[])
{
	//printf("main");
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
