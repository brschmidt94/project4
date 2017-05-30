#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"

int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *filename);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD);
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD);
int tfs_seek(fileDescriptor FD, int offset);
int verifyFormat(char *filename);

int diskSize = 0; //KEEP TRACK OF THIS
//TODO KEEP TRACK OF SIZES
int mounted = -1;  //file descriptor of mounted file, or negative one if none mounted
int diskNum = 0;

int tfs_mkfs(char *filename, int nBytes) {
	char *format;
	diskSize = nBytes;
	
	diskNum = openDisk(fileName, nBytes);
	
	if(nBytes > 0) {
		format = calloc(BLOCKSIZE, sizeof(char));
		format[1] = 0x45;
		
		//set superblock
		format[0] = 1;  //block type
		format[2] = BLOCKSIZE;// pointer to root inode
		format[3] = 2 * BLOCKSIZE;// pointer to free block list?  others can keep format[3] as 0?
		writeBlock(diskNum, 0, format);
		
		//set root inode
		format[0] = 2;
		format[2] = NULL;
		format[3] = NULL;
		writeBlock(diskNum, 1, format);
		
		//set free blocks
		format[0] = 4;
		for(int block = 2; block < nBytes / BLOCKSIZE; block ++) {	
			writeBlock(diskNum, block, format);
		
		
		
		}
	
		//SET SUEPRBLOCK AND INODES HERE
	}
	
	closeDisk(diskNum);
	
  //TODO save size somewhere
	return diskNum;
}

int tfs_mount(char *filename) {
	int diskNum = -4; //ERROR: IMPROPER DISK FORMAT
	//Where is nBytes set?
	
	if(verifyFormat(filename))
		diskNum = openDisk(filename, diskSize);
	
	return diskNum;
}

int verifyFormat(char *filename) {
	
	
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
