#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libDisk.h"
#include "tinyFS.h"
#include "TinyFS_errno.h"

//TODO: whenever we put a number into our blocks...do we have to cast it to a char???
//TODO: why is size in inode block #2 not being updated...can't test readByte now

int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *filename);

int tfs_unmount(void);//-
// *make sure set diskNum or mounted properly and use consistent one throughout code
fileDescriptor tfs_openFile(char *name);// B //-
int tfs_closeFile(fileDescriptor FD); //K
int tfs_writeFile(fileDescriptor FD, char *buffer, int size); //K *set errors for getFreeBlock? why (char)
//K *check todos: need diskNum saved in mount
int tfs_deleteFile(fileDescriptor FD); //B
int tfs_readByte(fileDescriptor FD, char *buffer); //K *need filepointer to be set to -1 in openFile
int tfs_seek(fileDescriptor FD, int offset); //B
int verifyFormat(int filename); //K *check logic
int getFreeBlock();
int findFile(char *name);
void printDiagnostics(int diskNum);

//This is a dynamically allocated linked list of openfiles.
struct openFile {
	fileDescriptor fd; //The blockNum index of the inode for a file on the FS
  char *filename;
  struct openFile *next;
  int filepointer;  //ADDED THIS TO USE IN MY FUNCTIONS
};

int mounted = -1; //Do we need this?  //USED in unmount

//NOTE: Per the spec, only one disk is mounted at a time
int diskNum = 0; //Actual UNIX File descriptor of currently mounted file
int diskSize = 0; //The size in bytes of the currently mounted disk
//TODO KEEP TRACK OF SIZES
struct openFile *fileList = NULL;




int main(int argc, char** argv) {
	int disk = tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_BLOCK_SIZE);
	fileDescriptor fd;
	char *buffer =  calloc(BLOCKSIZE, sizeof(char));
	int i;
	for (i = 0; i < BLOCKSIZE; i++) {
		buffer[i] = i;
	}

	tfs_mount(DEFAULT_DISK_NAME);
	
	fd = tfs_openFile("cats");
	tfs_writeFile(fd, buffer, 256);  //should take up 2 free blocks because extra bytes in beginning

	char *buf = calloc(1, sizeof(char));
	for (i = 0; i < 256; i++) {
		tfs_readByte(fd, buf);
		printf("%hhx ", buf[0]);
	}
	printf("\n");

	printDiagnostics(diskNum);
	tfs_unmount();

	return 0;
}



void printDiagnostics(int diskNum) {
	char *data = calloc(BLOCKSIZE, sizeof(char));
	int z;
	int block;
	for(block = 0; block < DEFAULT_BLOCK_SIZE / BLOCKSIZE; block++) {
		readBlock(diskNum, block, data);
		
		if(data[0] == 1) {
			printf("SUPERBLOCK @ %d\n", block);
			printf("Magic Number: %d\n", data[1]);
			printf("Root Inode Address: %d\n", data[2]);
			printf("Empty Spot: %d\n", data[3]);
			printf("Free block address: %d\n", data[4]);
		}
		else if(data[0] == 2) {
			if(data[4] == '/') {
				printf("ROOT INODE @ %d\n", block);
				printf("Magic Number: %d\n", data[1]);
				printf("Inode List Address: %d\n", data[2]);
				printf("Empty Spot: %d\n", data[3]);
				printf("Name: %s\n", data + 4);
				memcpy(&z, data +15, 4);
				printf("Size: %d\n", z);
				printf("File extents list address: %d\n", data[14]);
			}
			else {
				printf("INODE @ %d\n", block);
				printf("Magic Number: %d\n", data[1]);
				printf("Inode List Address: %d\n", data[2]);
				printf("Empty Spot: %d\n", data[3]);
				printf("Name: %s\n", data + 4);
				memcpy(&z, data +15, 4);
				printf("Size: %d\n", z);
				printf("File extents list address: %d\n", data[14]);
			}
		}
		else if(data[0] == 3) {
			printf("FILE EXTENT @ %d\n", block);
			printf("Magic Number: %d\n", data[1]);
			printf("File Extents List Address: %d\n", data[2]); //ADDED
			printf("Empty Spot: %d\n", data[3]);
		}
		else if(data[0] == 4) {
			printf("FREE BLOCK @ %d\n", block);
			printf("Magic Number: %d\n", data[1]);
			printf("Next Free Block Address: %d\n", data[2]);
			printf("Empty Spot: %d\n", data[3]);
		}
		
		printf("\n");
	}
}

//GOOD (except for the size saving?)
int tfs_mkfs(char *filename, int nBytes) {
	int index;
	int block;
	char *format;
	diskSize = nBytes;
	
	if((diskNum = openDisk(filename, nBytes)) == -1)
		diskNum = -6; ////ERROR: Tried to make empty filesystem of size 0
	
	if(nBytes < 2) //We assume that a file of just superblock and root inode can be made
		diskNum = -4; //ERROR: FILESYSTEM SIZE TOO SMALL
	else {
		format = calloc(BLOCKSIZE, sizeof(char));
		format[1] = 0x45;

		//Format Superblock
		format[0] = 1; //Block type
		format[2] = 1; //Pointer to Root Inode		
		format[4] = 2; //Pointer to free blocks - this will change over time
		writeBlock(diskNum, 0, format);
		
		//Set Root Inode
		format[0] = 2; //Block type
		format[2] = -1; //Pointer to list of Inodes
		format[4] = '/'; //Name
		format[5] = '\0'; //Null char
		format[13] = 0; //empty
		format[14] = -1; //Pointer to list of file extents
		format[15] = 0;
		format[16] = 0;
		format[17] = 0;
		format[18] = 0; //15, 16, 17, 18 is int for size	
		//TODO: cast to characters!!!
		writeBlock(diskNum, 1, format);

		//Set block to Free Block format	
		format[0] = 4;
		
		for(index = 2; index < 15; index++)
			format[index] = 0; //Zero out format block from previous formatting
		
		//Set free blocks
		for(block = 2; block < nBytes / BLOCKSIZE; block++) {	
			if(block == (nBytes / BLOCKSIZE) - 1)
				format[2] = -1; //End of list
			else
				format[2] = block + 1; //Point to next block
		
			writeBlock(diskNum, block, format);
		}
	
		closeDisk(diskNum);
	}
	
  //TODO save size somewhere
	return diskNum;
}

//GOOD
int tfs_mount(char *filename) {
	int status = -4; //ERROR: IMPROPER DISK FORMAT

	status = openDisk(filename, 0); //We pass zero since its presumable ALREADY MADE

	//if the file is non-existant, can't mount
	if(status == -1)
		status = -7; //ERROR: MAKE/MOUNT NON EXISTANT FILE 
	else { //if the file is existant, check it is mountable (correct format)

		//if(verifyFormat(status) != 0) {
		//	closeDisk(status);
		//	status = -8; //ERROR: TRIED TO MOUNT FILE WITH WRONG FORMAT
		//}
	}

	//Set diskNum instance var and return it too
	diskNum = status;

	return status;
}

/* tfs_unmount(void) "unmounts" the currently mounted file system
 * Only one file system may be mounted at a time.  
 * Use this to cleanly unmount the currently mounted file system.
 * Returns specified success/error code.
 */
//GOOD
 int tfs_unmount(void) {
 	int status = 0;

 	if (mounted == -1) {
 		//TODO: set errno /status
 		status = -1;
	}
	else {
 		closeDisk(mounted);
 		mounted = -1;
 	}

 	return status;
 }
 
 //GOOD
fileDescriptor tfs_openFile(char *name) {
	int status = -1;
	struct openFile *endOfList = NULL;
	int inodeIndex = 0;
	
	
	//Look in currently mounted FS
	//We can open multiple files, so we need to keep track of what we've
	//openened. For this we can use an array or a LL. The spec says "DYNAMIC",
	//so I imagine he wants a LL since we can malloc and free it.
	
	if(strlen(name) >= 8)
		return -11; //STRING TOO LONG
	

	
	//Iterate to spot in list right here
	if(fileList == NULL) { //File list currently empty. Allocate LL head.
		fileList = (struct openFile *) malloc(sizeof(struct openFile));
		fileList->next = NULL;
		endOfList = fileList;
	}
	else {
		endOfList = fileList;
		
		//Iterate to end of list, do we need to check if we already have the file?
		while(endOfList->next != NULL)
			endOfList = endOfList->next;
		
		endOfList->next = (struct openFile *) malloc(sizeof(struct openFile));
		endOfList = endOfList->next;
		endOfList->next = NULL;
	}

	//endOfList now points to a fresh file entry
	//Set the struct attributes right here

	if((inodeIndex = findFile(name)) != -1) {
		endOfList->fd = inodeIndex;
 		endOfList->filename = malloc(sizeof(char) * 9);
		strcpy(endOfList->filename, name);
		endOfList->filepointer = -1;
	}
	else
		inodeIndex = -12; //ERROR: FS is full (Cannot allocated inode)

	return inodeIndex;
}

//GOOD
int findFile(char *name) {
	char *block = calloc(sizeof(char), BLOCKSIZE);
	int nextInode = 1; //This is the INDEX of the next linked inode we read in.
	int fileExists = 0;
	int newFile = 0;
	int fileInode = -1;
	
	//We either find a file through list of inodes
	//or create a new inode
	
	readBlock(diskNum, 1, block); //Read in root inode

	while(!fileExists && !newFile) {			
		if(strcmp(block + 4, name) == 0)
			fileExists = 1;
		else if(block[2] == -1)
			newFile = 1;
		else {
			nextInode = block[2];
			readBlock(diskNum, nextInode, block);
		}
	}
	

	if(fileExists)
		fileInode = nextInode;
	else if(newFile) {
		fileInode = getFreeBlock();
		
		if(fileInode != -1) {
			block[2] = fileInode; //Attached new inode to end of inode list
			writeBlock(diskNum, nextInode, block); //Update second to last block
			
			block[2] = -1;
			strcpy(block + 4, name);
			block[13] = 0;
			writeBlock(diskNum, fileInode, block); //Append new block to end of list
		}
	}

	//This is the absolute index of the inode on the file. We either opened it or created it.
  return fileInode;
}

//GOOD
int getFreeBlock() {
	int freeBlockIndex = -1;
	char *superBlock = calloc(sizeof(char), BLOCKSIZE);
	char *blockToUse = calloc(sizeof(char), BLOCKSIZE);
	int freeBlock;
	
	readBlock(diskNum, 0, superBlock); //Read in superblock
	
	//If there is a freeblock for allocate an inode for
	if(superBlock[4] != -1) {
		freeBlock = superBlock[4]; //Save index of freeblock
		readBlock(diskNum, freeBlock, blockToUse);
		
		superBlock[4] = blockToUse[2]; //Patch the gap in the Linked list  //CHANGED from 4 to 2!
		writeBlock(diskNum, 0, superBlock);	
	}
	
	free(superBlock);
	free(blockToUse);
	
	return freeBlock; //We return the absolute index of the freeblock
}

//GOOD
int tfs_closeFile(fileDescriptor FD) {
	struct openFile *prev = fileList;
	struct openFile *next = fileList;
	int status = 0;
	int closed = 0;
	
	//If there's just one element in the list
	if(prev == NULL) //Filelist empty
		status = -21; 
	else if(prev->fd == FD) {
		free(prev->filename);
		fileList = prev->next;
	}
	else if (prev->next != NULL) {
		next = prev->next;
			
		while(!closed && next != NULL) {
			if(next->fd == FD) {
				free(next->filename);
				prev->next = next->next;
					
				closed = 1;
			}
			else {
				next = next->next;
				prev = prev->next;
			}
		}
	}
	
	return status;
}

/*
 * Pass in the fd of the file system being mounted.  The file has been opened, but must 
 * have 0x45 as the magic number in each block to be mounted.
 * Return 0 if correct format, and -1 otherwise.
 */
/**
int verifyFormat(int diskNum) {
	int ret = 0;
	char *buff = calloc(BLOCKSIZE, sizeof(char));
	char *extents = calloc(BLOCKSIZE, sizeof(char));
	int index = 0;
	int validRead = 0;

	validRead = readBlock(diskNum, 0, buff); // read in superblock

	//verify magic number is in each block
	if (validRead >= 0 && buff[1] == 0x45) { //if reading superblock  doesn't cause error
		//save index of free blocks
		int freeind = buff[4];

		//verify inodes and file extents 
		//(block 2 (third block in) points to inodes)
		//block 14 in inodes points to file extents
		while (buff[2] != -1) { //loop through inodes
			validRead = readBlock(diskNum, buff[2], buff);
			if (validRead < 0 || buff[1] != 0x45)
				return -1;

			//buff has inode
			validRead = readBlock(diskNum, buff[14], extents);
			if (validRead < 0 && extents[1] != 0x45) 
				return -1;

			while (extents[14] != -1) {  //loop through file extents
				validRead = readBlock(diskNum, extents[2], extents); //TODO: is 2 right index???
				if (validRead < 0 || extents[1] != 0x45)
					return -1;
			}
			
		}

		//verify free blocks
		validRead = readBlock(diskNum, freeind, buff);
		if (validRead < 0 && buff[1] != 0x45) 
			return -1;
		while (buff[2] != -1) {
			if (validRead < 0 || buff[1] != 0x45)
				return -1;

		}
	} else 
		ret = -1;
	
	return ret;
}

*/

/*
 * Writes buffer 'buffer' of size 'size', which represents an entire file's content, 
 * to the file system.  Sets the file pointer to 0 (the start of file) when done.  
 * Returns success/error codes.
 */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
	int status = 0;
	char *block = calloc(sizeof(char), BLOCKSIZE);
	struct openFile *file = fileList;
	int ind = 0;
	int found = 0;

	if (size >= BLOCKSIZE) 
		status = -1; //break
	//make sure the file is open
	if(fileList == NULL) {
		//TODO: set error, for file not opened yet.
		//set status
		printf("ERROR 1 in write file\n");
	} 

	do {
		if (file->fd == FD) {
			found = 1;
		} else {
			file = file->next;
		}
	} while (file->next != NULL && !found);

	if (found) {
		//printf("FOUND write file\n");
		//get the inode to get address
		readBlock(diskNum, FD, block);  //block = inode
		int freeind = getFreeBlock();
		memcpy(block + 15, &size, sizeof(unsigned int));
		//printf("wrote size as %u to %d\n", (unsigned char)block[13], FD);
		block[14] = (unsigned char) freeind;  //inode[14] points to file extents
		writeBlock(diskNum, FD, block); //set pointer in inode to next data block
		file->filepointer = 0;
		int ind;
		int last = 0;
		//round up division?
		for (ind = 0; ind < ((size + (BLOCKSIZE - 4) - 1) / (BLOCKSIZE - 4)); ind++) {
			if (ind == ((size + (BLOCKSIZE - 4) - 1) / (BLOCKSIZE - 4)) - 1)
				last = 1;

			readBlock(diskNum, freeind, block);  //read free block

			if (last) {
				block[0] = 3;
				block[2] = -1;
				block[3] = 0;
				//write data
				memcpy(block + 4, ind *(BLOCKSIZE - 4) + buffer, size - (BLOCKSIZE - 4) * ind);
				writeBlock(diskNum, freeind, block);
			} else {
				int  nextind = getFreeBlock();
				block[0] = 3;
				block[2] = nextind;
				block[3] = 0;
				memcpy(block + 4, ind *(BLOCKSIZE - 4) + buffer, BLOCKSIZE - 4);
				writeBlock(diskNum, freeind, block);
				freeind = nextind;
			}

		}

	} else {
		//TODO: set error for file not opened
		//set status
		printf("Error 2 in writeFile\n");
	}

	return status;
}


/*
 * Reads one byte from the file and copies it to buffer, using the current
 * file pointer location and incrementing it by one upon success.  If the file
 * pointer is already at the end of the file, then tsf_readByte() should return
 * an error and not increment the file pointer.
 */
int tfs_readByte(fileDescriptor FD, char *buffer) {
	int status = 0;
	char *block = calloc(sizeof(char), BLOCKSIZE);
	struct openFile *file = fileList;
	int found = 0;

	//make sure the file is open
	if(fileList == NULL) {
		//TODO: set error, for file not opened yet.
		//set status
		printf("ERROR 1 in readbyte file\n"); //TODO: remove later
	} 

	do {
		if (file->fd == FD) {
			found = 1;
		} else {
			file = file->next;
		}
	} while (file->next != NULL && !found);

	if (!found) 
		status = -1;//set error...figure out details: TODO

	//file holds filepointer...maybe move to inode?  oh well
	//get which file extent:
	int ext = file->filepointer / (BLOCKSIZE - 4); //which extent to go to
	int extind = (file->filepointer - (BLOCKSIZE - 4) * ext) + 4;  //index into the correct extent
	readBlock(diskNum, FD, block);  //block = inode

	int z;
	memcpy(&z, block +15, 4);
	if (z <= file->filepointer)  
		status = -1;
	else {
		int nextext = block[14]; //points to first extent

		int i;
		//get index to correct extent
		for (i = 0; i < ext; i++) {
			readBlock(diskNum, nextext, block);
			nextext = block[2];
		}
		//read in correct extent
		readBlock(diskNum, nextext, block);
		memcpy(buffer, block + extind, 1);
		file->filepointer++;
	}

	return status;
}