#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"

int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *filename);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);// B
int tfs_closeFile(fileDescriptor FD); //K
int tfs_writeFile(fileDescriptor FD); //K
int tfs_deleteFile(fileDescriptor FD); //B
int tfs_readByte(fileDescriptor FD); //K
int tfs_seek(fileDescriptor FD, int offset); //B
int verifyFormat(char *filename); //K

int diskSize = 0; //KEEP TRACK OF THIS
//TODO KEEP TRACK OF SIZES
int mounted = -1;  //file descriptor of mounted file, or negative one if none mounted
int diskNum = 0;

int tfs_mkfs(char *filename, int nBytes) {
	char *format;
	diskSize = nBytes;
	
	diskNum = openDisk(fileName, nBytes);
	
	if(diskNum == -1)
		diskNum = -6; ////ERROR: Tried to make empty filesystem of size 0
	
	if(nBytes > 2) {
		format = calloc(BLOCKSIZE, sizeof(char));
		format[1] = 0x45;
		
		//set superblock
		format[0] = 1;  //block type
		format[2] = BLOCKSIZE;// pointer to root inode
		format[3] = BLOCKSIZE * 2; //Pointer to free blocks - this will change over time
		writeBlock(diskNum, 0, format);
		
		//set root inode
		format[0] = 2;
		format[2] = NULL; // We don;t have anything yet THERE ARE INDOES
		format[3] = 0; //[EMPTY]
		format[4] = "/";
		format[5] = "\0";
		format[13] = 0; 
		format[14] = NULL; //file extents
		
		writeBlock(diskNum, 1, format);
		
		//set free blocks
		format[0] = 4;
		for(int block = 2; block < nBytes / BLOCKSIZE; block ++) {	
			if(block == (nBytes / BLOCKSIZE) - 1)
				format[2] = NULL;
			else
				format[2] = (block * BLOCKSIZE) + 1;
		
			writeBlock(diskNum, block, format);
		
			closeDisk(diskNum);
		}
		else
			diskNum = -4; //ERROR: FILESYSTEM SIZE TOO SMALL
	
  //TODO save size somewhere
	return diskNum;
}

int tfs_mount(char *filename) {
	int diskNum = -4; //ERROR: IMPROPER DISK FORMAT
	//Where is nBytes set?
	
	if(verifyFormat(filename)) {
		diskNum = openDisk(filename, 0); //We pass zero since its ALREADY MADE
			
		if(diskNum == -1)
			diskNum = -7; //ERROR: MAKE/MOUNT NON EXISTANT FILE 
	}
	
	return diskNum;
}

int verifyFormat(char *filename, ) {
	//KIRSTEN
	//ITERATE THRU LINKED LISTS
	
	return 1;
}

/* tfs_unmount(void) "unmounts" the currently mounted file system
 * Only one file system may be mounted at a time.  
 * Use this to cleanly unmount the currently mounted file system.
 * Returns specified success/error code.
 */
 int tfs_unmount(void) {
 	int status = 0;

 	if (mounted == -1) {
 		//TODO: set errno /status
 		status = -1;
 	} else {
 		closeDisk(mounted);
 		mounted = -1;
 	}

 	return status;
 }
