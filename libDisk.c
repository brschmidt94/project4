#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 
#include "tinyFS.h"
#include "TinyFS_errno.h"
#include "libDisk.h"

int openDisk(char *filename, int nBytes) {
	FILE *disk;
	int diskNum;

	if(nBytes >= 0 && nBytes % BLOCKSIZE == 0) {
		if(nBytes == 0 && access(filename, F_OK) != 0)
			diskNum = FILESYSTEM_NOT_FOUND_OR_OF_SIZE_ZERO;
		else if(nBytes == 0) //Open, do not clear
			disk = fopen(filename, "r+");
		else //Open and clear
			disk = fopen(filename, "w");
						
		diskNum = fileno(disk);
	}
	else
		diskNum = MISALIGNED_BLOCKSIZE; //ERROR: nBytes not integral with BLOCKSIZE
	
	return diskNum;
}

int readBlock(int disk, int bNum, void *block) {
	int status = 0; //Stores error/success state
	
	//Read into buff
	if(pread(disk, block, BLOCKSIZE, bNum * BLOCKSIZE) == -1) 
		status = READ_FAILURE;

	return status;
}

int writeBlock(int disk, int bNum, void *block) {
	int status = 0; //Stores error/success state
	
	if(pwrite(disk, block, BLOCKSIZE, bNum * BLOCKSIZE) == -1)
		status = WRITE_ERROR;
	
	return status;
}

/*
 * Disk is file descriptor.
 */
void closeDisk(int disk) {
	close(disk);
}
