#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"

int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *filename);

int tfs_unmount(void);
// *make sure set diskNum or mounted properly and use consistent one throughout code
fileDescriptor tfs_openFile(char *name);// B
int tfs_closeFile(fileDescriptor FD); //K
int tfs_writeFile(fileDescriptor FD, char *buffer, int size); //K *set errors for getFreeBlock? why (char)
//K *check todos: need diskNum saved in mount
int tfs_deleteFile(fileDescriptor FD); //B
int tfs_readByte(fileDescriptor FD, char *buffer); //K *need filepointer to be set to -1 in openFile
int tfs_seek(fileDescriptor FD, int offset); //B
int verifyFormat(int filename); //K *check logic

//This is a dynamically allocated linked list of openfiles.
struct openFile {
	fileDescriptor fd; //The blockNum index of the inode for a file on the FS
  char *filename;
  struct openFile *next;
  char *filepointer;  //ADDED THIS TO USE IN MY FUNCTIONS
};




int mounted = -1; //Do we need this?  //USED in unmount

//NOTE: Per the spec, only one disk is mounted at a time
int diskNum = 0; //Actual UNIX File descriptor of currently mounted file
int diskSize = 0; //The size in bytes of the currently mounted disk
//TODO KEEP TRACK OF SIZES
struct openFile *fileList = NULL;

int tfs_mkfs(char *filename, int nBytes) {
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
		//It is simple to not store pointers as BLOCKNUM * BLOCKSIZE. Just use a normal index integer
		format[4] = 2; //Pointer to free blocks - this will change over time
		////////////////////////////////////////////////////////////
		writeBlock(diskNum, 0, format);
		
		//Set Root Inode
		format[0] = 2; //Block type
		format[2] = -1; //Pointer to list of Inodes. We dont use NULL because it isnt a memory address

		format[4] = '/'; //Name
		format[5] = '\0'; //Null char

		format[13] = 0; //File size
		format[14] = -1; //Pointer to list of file extents	
		writeBlock(diskNum, 1, format);
		
		//Set block to Free Block format	
		format[0] = 4;
		int index;
		for(index = 2; index < 15; index++)
			format[index] = 0; //Zero out format block from previous formatting
		
		//Set free blocks
		int block;
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

int tfs_mount(char *filename) {
	int diskNum = -4; //ERROR: IMPROPER DISK FORMAT

	diskNum = openDisk(filename, 0); //We pass zero since its presumable ALREADY MADE
	//TODO....want to set global diskNum

	//if the file is non-existant, can't mount
	if(diskNum == -1) {
		diskNum = -7; //ERROR: MAKE/MOUNT NON EXISTANT FILE 
	} else { //if the file is existant, check it is mountable (correct format)
		//I moved verify format AFTER open disk because it needs to be opened to read and verify it...
		//close disk if it is not mountable (ie if it doesn't have the magic numbers)
		//return an error
		if(verifyFormat(diskNum) != 0) {
			closeDisk(diskNum);
			diskNum = -8; //ERROR: TRIED TO MOUNT FILE WITH WRONG FORMAT
		}
	}

	
	return diskNum;
}

/*
 * Pass in the fd of the file system being mounted.  The file has been opened, but must 
 * have 0x45 as the magic number in each block to be mounted.
 * Return 0 if correct format, and -1 otherwise.
 */
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
	} else {
		ret = -1;
	}
	return ret;
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
 
 
fileDescriptor tfs_openFile(char *name) {
	int status = -1;
	struct openFile *endOfList = NULL;
	int inodeIndex = 0;
	
	
	//Look in currently mounted FS
	//We can open multiple files, so we need to keep track of what we've
	//openened. For this we can use an array or a LL. The spec says "DYNAMIC",
	//so I imagine he wants a LL since we can malloc and free it.
	
	if(strlen(name) == 8)
		return -11; //STRING TOO LONG
	
	//Iterate to spot in list right here
	if(fileList == NULL) { //File list currently empty. Allocate LL head.
		fileList = (struct openFile *) malloc(sizeof(struct openFile));
		fileList->next = NULL;

	}
	else {
		endOfList = fileList;
		
		//Check if file already exists here
		while(endOfList->next != NULL)
			endOfList = endOfList->next;
		
		endOfList->next = malloc(sizeof(struct openFile));
		endOfList = endOfList->next;
		endOfList->next = NULL;
	}

	//endOfList now points to a fresh file entry
	//Set the struct attributes right here
	

	if((inodeIndex = findFile(name)) != -1) {
		endOfList->fd = inodeIndex;
 		endOfList->fileName = malloc(sizeof(char) * 9);
		strcpy(endOfList->filename, name);
		endOfList->filepointer = -1;
	}
	else
		inodeIndex = -12; //ERROR: FS is full (Cannot allocated inode)

	return inodeIndex;
}

int findFile(char *name) {
	char *block = calloc(sizeof(char), BLOCKSIZE);
	int nextInode = 1; //This is the INDEX of the next linked inode we read in.
	//Note that this is the ABSOLUTE index and not an offset.
	int fileExists = 0;
	int newFile = 0;
	
	//We either find a file through list of inodes
	//or create a new inode
	
	readBlock(diskNum, 1, block); //Read in root inode

	while(!fileExists && !newFile) {
			
		if(strcmp(block + 4, name))
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
		//In this case we will need to append an inode to nextInode and return the
		//index of that new indode
		
		fileInode = getFreeBlock();
		
		if(fileInode != -1) {
			block[2] = fileInode; //Attached new inode to end of inode list
			
			//Update second to last block
			writeBlock(diskNum, nextInode, block);
			
			block[2] = -1;
			strcpy(block + 4, name);
			block[13] = 0;
			
			//Alocate new block
			writeBlock(diskNum, fileInode, block);
		}
	}

	//This is the absolute index of the inode on the file. We either opened it or created it.
  return fileInode;
}

int getFreeBlock(char *name) {
	int freeBlockIndex = -1;
	char *block = calloc(sizeof(char), BLOCKSIZE);
	char *freeBlock;
	
	readBlock(diskNum, 0, block); //Read in superblock
	
	//If there is a freeblock for allocate an inode for
	if(block[3] != -1) {
		freeBlock = block[3]; //Save index of freeblock
		
		freeBlockIndex = calloc(sizeof(char), BLOCKSIZE);
		readBlock(diskNum, freeBlockIndex, BLOCKSIZE);
		
		block[3] = freeBlockIndex[3]; //Patch the gap in the Linked list	
	}
	
	return freeBlock;
}


/*
 * Writes buffer 'buffer' of size 'size', which represents an entire file's content, 
 * to the file system.  Sets the file pointer to 0 (the start of file) when done.  
 * Returns success/error codes.
 */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
	int status = 0;
	char *block = calloc(sizeof(char), BLOCKSIZE);
	//FD is blocknum index for inode for file on mounted file...make sure file is open
	if(fileList == NULL) {
		//TODO: set error, for file not opened yet.
		//set status
	} 

	struct openFile *file = fileList;
	int ind = 0;
	int found = 0;
	while (file->next != NULL && !found) {
		if (file->fd == FD) {
			found = 1;
		} else {
			file = file->next;
		}
	}

	if (found) {
		//TODO: make sure diskNum was set properly in mount as global...not local
		//format[14] pointes to data
		//get the inode to get address
		readBlock(diskNum, FD, block);  //block = inode
		int freeind = getFreeBlock();
		block[14] = freeind;
		writeBlock(diskNum, FD, block); //set pointer in inode to next data block
		file->filepointer = freeind + 4; 			 //does this change the actual thing in the list?
				//note - it points the the data, not first bytes
		int ind;
		int last = 0;
		//round up division?
		for (ind = 0; ind < ((size + (BLOCKSIZE - 4) - 1) / (BLOCKSIZE - 4)); ind++) {
			if (ind == ((size + (BLOCKSIZE - 4) - 1) / (BLOCKSIZE - 4)) - 1)
				last = 1;

			readBlock(diskNum, freeind, block);  //read free block

			if (last) {
				block[2] = -1;
				//write data
				memcopy(block + 4, ind *(BLOCKSIZE - 4) + buffer, size - (BLOCKSIZE - 4) * ind);
				writeBlock(diskNum, freeind, block);
			} else {
				int  nextind = getFreeBlock();
				block[2] = nextind;
				memcopy(block + 4, ind *(BLOCKSIZE - 4) + buffer, BLOCKSIZE - 4);
				writeBlock(diskNum, freeind, block);
				freeind = nextind;
			}

		}

	} else {
		//TODO: set error for file not opened
		//set status
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

	return 0;
}
