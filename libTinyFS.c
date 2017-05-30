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

int diskSize = 0;
int mounted = -1;  //file descriptor of mounted file, or negative one if none mounted


int tfs_mkfs(char *filename, int nBytes) {
	diskSize = nBytes;
	
	
	
	return 1;
}

int tfs_mount(char *filename) {
	int diskNum = -1;
	//Where is nBytes set?
	
	if(verifyFormat(filename))
		diskNum = openDisk(filename, nBytes);
	else
		diskNum = -4; //ERROR: IMPROPER DISK FORMAT
	
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
