#include <stdio.h>
#include <unistd.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"
#include "libDisk.h"

int openDisk(char *filename, int nBytes) {
	FILE disk;
	
	//is nBytes the # of BLOCKS? or the number of bytes of the overall file
	//% 256 = 0
	
	if(access(filename, F_OK) != -1) { //If the file already exists
		if(nBytes > 0) //Overwrite existing file
			disk = fopen(filename, "w+");
		else//nBytes == 0. Open existing file as read-only
			disk = fopen(filename, "r");
	}
	else //We are making a new file
		disk = fopen(filename, "w");

	return fileno(disk);
}

int readBlock(int disk, int bNum, void *block) {
	int status = 0; //Stores error/success state
	int fileOffset;
	
	//Index into file by block number
	if(fileOffset = lseek(disk, bNum * BLOCKSIZE, SEEK_CUR) >= 0)
		status = fread(block, sizeof(char), bNum * BLOCKSIZE, fileOffset);
	
	return status;
}

int writeBlock(int disk, int bNum, void *block) {
	int status = 0; //Stores error/success state
	int fileOffset;
	
	//Index into file by block number
	if(fileOffset = lseek(disk, bNum * BLOCKSIZE, SEEK_CUR) >= 0)
		status = fwrite(block, sizeof(char), bNum * BLOCKSIZE, fileOffset);
	
	return status;
}

void closeDisk(int disk) {
	fclose(disk);
}
