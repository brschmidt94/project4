#include <stdio.h>
#include <unistd.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"
#include "libDisk.h"

int openDisk(char *filename, int nBytes) {
	FILE *disk;
	int diskNum;
	
	//is nBytes the # of BLOCKS? or the number of bytes of the overall file
	//% 256 = 0
	
	if(nBytes < 0 || nBytes % BLOCKSIZE != 0) {
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
	if(pread(disk, block, BLOCKSIZE, bNum * BLOCKSIZE) == -1) //changed to seek_set from seek_cur bc offset from beginning always?
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

void closeDisk(int disk) {
	fclose(disk);
}

