#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"
#include "libDisk.h"

int openDisk(char *filename, int nBytes) {
	FILE *disk;
	int diskNum;
		
	if(nBytes >= 0 && nBytes % BLOCKSIZE == 0) {
		if(access(filename, F_OK) != -1) { //If the file already exists
			if(nBytes > 0) //Overwrite existing file
				disk = fopen(filename, "w+");
			else//nBytes == 0. Open existing file as read-only
				disk = fopen(filename, "r"); //Do not overwrite
		}
		else //We are making a new file
			disk = fopen(filename, "w");
		
		diskNum = fileno(disk);
	}
	else
		diskNum = -1; //ERROR: nBytes not integral with BLOCKSIZE
		
	return diskNum;
}

int readBlock(int disk, int bNum, void *block) {
	int status = 0; //Stores error/success state
	
	//Read into buff
	if(pread(disk, block, BLOCKSIZE, bNum * BLOCKSIZE) == -1) 
		status = -1;//TODO: handle error
		//status = read(block, sizeof(char), bNum * BLOCKSIZE, fileOffset);
	
	return status;
}

int writeBlock(int disk, int bNum, void *block) {
	int status = 0; //Stores error/success state
	
	if(pwrite(disk, block, BLOCKSIZE, bNum * BLOCKSIZE) == -1)
		status = -3; //WRITE_ERROR errno
	
	return status;
}

/*
 * Disk is file descriptor.
 */
void closeDisk(int disk) {
	close(disk);
}

int main() {
	int disk;
	
	char *buff = calloc(BLOCKSIZE, sizeof(char));
	memcpy(buff, "Bradley Schmidt", 16);
	
	disk = openDisk("a.txt", BLOCKSIZE);
	writeBlock(disk, 0, buff);
	close(disk);
	
	return 0;
}

