#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libDisk.h"
#include "tinyFS.h"
#include "TinyFS_errno.h"

//This is a dynamically allocated linked list of currently open files.
struct openFile {
	fileDescriptor fd; //The absolute index of an inode on the FS
  char *filename; //The name of the open file
  struct openFile *next; //Pointer to next openFile
  int filepointer; //Index a pointer into the file (used in tfs_readByte() and tfs_seek())
};

int mounted = 0; //Indicates whether a disk is currently mounted
int diskNum = 0; //Actual UNIX File descriptor of currently mounted file
int diskSize = 0; //The size in bytes of the currently mounted disk
struct openFile *fileList = NULL; //Dynamic list of open files

time_t timer;
char time_buffer[26];
struct tm* tm_info;

void printDiagnostics(int diskNum) {
	char *data = calloc(BLOCKSIZE, sizeof(char));
	int z;
	int block;
	for(block = 0; block < DEFAULT_BLOCK_SIZE / BLOCKSIZE; block++) {
		readBlock(diskNum, block, data);
		
		if(data[0] == 1) {
			printf("SUPERBLOCK @ %d\n", block);
			printf("Magic Number: 0x%x\n", data[1]);
			printf("Root Inode Address: %d\n", data[2]);
			printf("Empty Spot: %d\n", data[3]);
			printf("Free block address: %d\n", data[4]);
		}
		else if(data[0] == 2) {
			if(data[4] == '/') {
				printf("ROOT INODE @ %d\n", block);
				printf("Magic Number: 0x%x\n", data[1]);
				printf("Inode List Address: %d\n", data[2]);
				printf("Empty Spot: %d\n", data[3]);
				printf("Name: %s\n", data + 4);
				memcpy(&z, data +15, 4);
				printf("Size: %d\n", z);
				printf("File extents list address: %d\n", data[14]);
			}
			else {
				printf("INODE @ %d\n", block);
				printf("Magic Number: 0x%x\n", data[1]);
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
			printf("Magic Number: 0x%x\n", data[1]);
			printf("File Extents List Address: %d\n", data[2]);
			printf("Empty Spot: %d\n", data[3]);
		}
		else if(data[0] == 4) {
			printf("FREE BLOCK @ %d\n", block);
			printf("Magic Number: 0x%x\n", data[1]);
			printf("Next Free Block Address: %d\n", data[2]);
			printf("Empty Spot: %d\n", data[3]);
		}
		
		printf("\n");
	}
}

int tfs_mkfs(char *filename, int nBytes) {
	int index;
	int block;
	char *format;
	diskSize = nBytes; //Save size to instance variable
	
	time(&timer);
	
	diskNum = openDisk(filename, nBytes);
	
	if(nBytes < 2) //We assume that a file of just superblock and root inode can be made
		diskNum = FILESYSTEM_TOO_SMALL; //ERROR: FILESYSTEM SIZE TOO SMALL
	else {
		format = calloc(BLOCKSIZE, sizeof(char));
		format[1] = 0x45;

		//Format Superblock ///////////////////////////////////////////////////////////
		format[0] = (char) 1; //Block type
		format[2] = (char) 1; //Pointer to Root Inode		
		format[4] = (char) 2; //Pointer to free blocks - this will change over time
		
		writeBlock(diskNum, 0, format);
		
		//Set Root Inode //////////////////////////////////////////////////////////////
		format[0] = (char) 2; //Block type
		format[2] = (char) -1; //Pointer to list of Inodes
		format[4] = '/'; //Name
		format[5] = '\0'; //Null char
		format[13] = (char) 0; //Permissions byte. value accroding to R, W, RW
		//ro = 0, rw = 1
		format[14] = (char) -1; //Pointer to list of file extents
		format[15] = (char) 0;
		format[16] = (char) 0;
		format[17] = (char) 0;
		format[18] = (char) 0; //15, 16, 17, 18 is int for size	
		
		//format[19] - format[44] holds CREATION time
		tm_info = localtime(&timer);
		strftime(format + 19, 26, "%H:%M:%S", tm_info);		
		//format[45] - format[70] holds MODIFIED time
		format[45] = '\0';
		//format[71] - format[95] holds ACCESSED time
		format[71] = '\0';
		
		writeBlock(diskNum, 1, format);

		//Set block to Free Block format //////////////////////////////////////////////
		format[0] = (char) 4;
		
		for(index = 2; index < 15; index++)
			format[index] = (char) 0; //Zero out format block from previous formatting
		
		//Set free blocks
		for(block = 2; block < nBytes / BLOCKSIZE; block++) {	
			if(block == (nBytes / BLOCKSIZE) - 1)
				format[2] = (char) -1; //End of list
			else
				format[2] = (char) block + 1; //Point to next block
		
			writeBlock(diskNum, block, format);
		}
	
		closeDisk(diskNum);
		free(format);
	}
	
	return diskNum;
}

int tfs_mount(char *filename) {
	int status = -4; //ERROR: IMPROPER DISK FORMAT

	if((status = openDisk(filename, 0)) >= 0) { //if the file is existant, check it is mountable (correct format)
		if(verifyFormat(status) != 0) {
			closeDisk(status);
			status = BAD_FILESYSTEM_FORMAT;
		} else {
			//Set diskNum instance var and return it too
			diskNum = status;
			mounted = 1;
		}
	}

	return status;
}

/* tfs_unmount(void) "unmounts" the currently mounted file system
 * Only one file system may be mounted at a time.  
 * Use this to cleanly unmount the currently mounted file system.
 * Returns specified success/error code.
 */
 int tfs_unmount(void) {
 	int status = 0;

 	if (!mounted)
 		status = NO_FILESYSTEM_MOUNTED;
	else {
 		closeDisk(diskNum);
 		mounted = 0;
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
	
	if(strlen(name) >= 8)
		return FILENAME_TOO_LONG; //STRING TOO LONG
	
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
		inodeIndex = FILESYSTEM_IS_FULL;

	return inodeIndex;
}

int findFile(char *name) {
	char *block = calloc(sizeof(char), BLOCKSIZE);
	int nextInode = 1; //This is the INDEX of the next linked inode we read in.
	int fileExists = 0;
	int newFile = 0;
	int fileInode = -1;
	int index = 0;
	
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
			
			block[0] = (char) 2;
			block[2] = -1;
			strcpy(block + 4, name);
			
			block[13] = (char) 1; //Permissions byte RW
			
			//Zero out data from before
			for(index = 15; index < 19; index++)
				block[index] = (char) 0;
			
			tm_info = localtime(&timer);
			strftime(block + 19, 26, "%H:%M:%S", tm_info);
			
			writeBlock(diskNum, fileInode, block); //Append new block to end of list
		}
	}
	
	free(block);

	//This is the absolute index of the inode on the file. We either opened it or created it.
  return fileInode;
}

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
		
		superBlock[4] = blockToUse[2]; //Patch the gap in the Linked list  
		writeBlock(diskNum, 0, superBlock);	
	}
	
	free(superBlock);
	free(blockToUse);
	
	return freeBlock; //We return the absolute index of the freeblock
}

int tfs_closeFile(fileDescriptor FD) {
	struct openFile *prev = fileList;
	struct openFile *next = fileList;
	int status = 0;
	int closed = 0;
	
	//If there's just one element in the list
	if(prev == NULL) //Filelist empty
		status = NO_FILES_IN_FILESYSTEM; 
	else if(prev->fd == FD) {
		free(prev->filename);
		fileList = prev->next;
	}
	else if (prev->next != NULL) {
		next = prev->next;
			
		while(!closed && next != NULL) {
			if(next->fd == FD) {
				prev->next = next->next;
				free(next->filename);
					
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

//change the inode pointer to point to the offset instead of the first file extent
int tfs_seek(fileDescriptor FD, int offset) {
	struct openFile *file = fileList;
	int status = FILE_NOT_FOUND;
	int foundFile = 0;
	
	if(file == NULL)
		status = EMPTY_FILE;
	else {
		while(!foundFile && file) {
			if(file->fd == FD) {
				foundFile = 1;
				status = 0;
				
				file->filepointer = offset;
			}
			else
				file = file->next;
		}
	}
	
	return status;
}


int tfs_rename(char *name, char *newName) {
	char *block = calloc(sizeof(char), BLOCKSIZE);
	int foundFile = 0;
	int blockNum = 1;
	int nextInode = 0;
	int status = 0;
	
	readBlock(diskNum, 1, block);
	
	if(!(strlen(newName) > 0 && strlen(newName) <= 8))
		return BAD_FILENAME_LENGTH;
	
	while(!foundFile) {			
		if(strcmp(block + 4, name) == 0) {
			foundFile = 1;
			
			memcpy(block + 4, newName, 9);
			writeBlock(diskNum, nextInode, block);
			
			status = 0;
		}
		else {
			if((nextInode = block[2]) != -1)
				readBlock(diskNum, nextInode, block);
			else {
				foundFile = 1;
				status = FILE_DOES_NOT_EXIST;
			}
		}
	}
	
	free(block);
	
	return status;
}

void tfs_readdir() {
	char *block = calloc(sizeof(char), BLOCKSIZE);
	int done = 0;
	
	//List Root Inode
	readBlock(diskNum, 1, block);
	printf("Directories:\n");
	printf("%s\n\n", block + 4);
	
	if(block[2] == -1)
		printf("No files currently in directory.\n");
	else {
		readBlock(diskNum, block[2], block);
		
		while(!done) {
			int z; //int we read in to avoid endianess issues
			memcpy(&z, block +15, 4);
			printf("%s - %d bytes\n", block + 4, z);

			if(block[2] == -1)
				done = 1;
			else
				readBlock(diskNum, block[2], block);
		}
	}
	
	free(block);	
}

void tfs_readFileInfo(fileDescriptor FD) {
	char *block = (char *) calloc(sizeof(char), BLOCKSIZE);
	struct openFile *file = fileList;
	int foundFile = 0;
	int status = 0;
	
	if(file == NULL) {
		printf("no files\n");
		status = EMPTY_FILE;
	}
	else {
		while(!foundFile && file) {
			if(file->fd == FD) {
				foundFile = 1;
					
				readBlock(diskNum, file->fd, block);
				int z; //int we read in to avoid endianess issues
				memcpy(&z, block +15, 4);
				printf("%s - %d bytes\n", block + 4, z);
				printf("Created: %s\n", block + 19);
				printf("Modified: %s\n", block + 45);
				printf("Accessed: %s\n", block + 71);				
			}
			else
				file = file->next;
		}
	}
	
	free(block);
}

//////////////////////// KIRSTEN

int tfs_makeRO(char *name) {
	char *block = calloc(sizeof(char), BLOCKSIZE);
	int foundFile = 0;
	int blockNum = 1;
	int nextInode = 0;
	int status = 0;
	
	readBlock(diskNum, 1, block); //read root inode
	
	while(!foundFile) {			
		if(strcmp(block + 4, name) == 0) {
			foundFile = 1;
			
			block[13] = (char) 0;
			writeBlock(diskNum, nextInode, block);
			
			status = 0;
		}
		else {
			if((nextInode = block[2]) != -1)
				readBlock(diskNum, nextInode, block);
			else {
				foundFile = 1;
				status = FILE_DOES_NOT_EXIST;
			}
		}
	}
	
	free(block);
	
	return status;
}

int tfs_makeRW(char *name) {
	char *block = calloc(sizeof(char), BLOCKSIZE);
	int foundFile = 0;
	int blockNum = 1;
	int nextInode = 0;
	int status = 0;
	
	readBlock(diskNum, 1, block); //read root inode
	
	while(!foundFile) {			
		if(strcmp(block + 4, name) == 0) {
			foundFile = 1;
			
			block[13] = (char) 1;
			writeBlock(diskNum, nextInode, block);
			
			status = 0;
		}
		else {
			if((nextInode = block[2]) != -1)
				readBlock(diskNum, nextInode, block);
			else {
				foundFile = 1;
				status = FILE_DOES_NOT_EXIST;
			}
		}
	}
	
	free(block);
	
	return status;
}

int tfs_writeByte(fileDescriptor FD, unsigned int data) {
	int status = 0;
	char *block = calloc(sizeof(char), BLOCKSIZE);
	struct openFile *file = fileList;
	int ind = 0;
	int found = 0;
	time(&timer);
	//make sure the file is open
	if(fileList == NULL) {
		status = -12; //file does not exist
	} 

	do {
		if (file->fd == FD) {
			found = 1;
		} else {
			file = file->next;
		}
	} while (file != NULL && !found);

	if (found) {
		//get which file extent:
		int ext = file->filepointer / (BLOCKSIZE - 4); //which extent to go to
		int extind = (file->filepointer - (BLOCKSIZE - 4) * ext) + 4;  //index into the correct extent
		readBlock(diskNum, FD, block);  //block = inode

		if (block[13]) {
			//update modified time
			tm_info = localtime(&timer);
			strftime(block + 45, 26, "%H:%M:%S", tm_info);
			writeBlock(diskNum, FD, block);

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
		
				//read in correct extent and write data
				readBlock(diskNum, nextext, block);
				block[extind] = (char) data; 
				writeBlock(diskNum, nextext, block);
				file->filepointer++;
			}
		} else
			status = MODIFYING_READ_ONLY_FILE;

	} else {
		status = -15; //file not opened
	}

	return status;
}









//READ ONLY AND WRITEBYTE SUPPORT GOES HERE



///////////////////////////////////////////

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
	if (buff[1] == 0x45) { //if reading superblock  doesn't cause error
		ret = 0;
//	return ret;
//}
		//save index of free blocks
		int freeind = buff[4];

		//verify inodes and file extents 
		//(block 2 (third block in) points to inodes)
		//block 14 in inodes points to file extents
		while (buff[2] != -1) { //loop through inodes
			readBlock(diskNum, buff[2], buff);
			if (buff[1] != 0x45)
				ret = -1;

			//buff has inode
			if (buff[14] != -1) {
				readBlock(diskNum, buff[14], extents);
				if (extents[1] != 0x45) 
					ret =  -1;

				while (extents[2] != -1) {  //loop through file extents
					validRead = readBlock(diskNum, extents[2], extents); //TODO: is 2 right index???
					if (extents[1] != 0x45)
						ret = -1;
				}
			}
	
		}

		//verify free blocks
		readBlock(diskNum, freeind, buff);
		if (buff[1] != 0x45) 
			ret = -1;
		while (buff[2] != -1) {
			if (buff[1] != 0x45)
				return -1;
			readBlock(diskNum, buff[2], buff);
		}

	} else 
			ret = -1;

	return ret;
}
	


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

	time(&timer);

	//make sure the file is open
	if(fileList == NULL) {
		status = -12; //file does not exist
	} 

	do {
		if (file->fd == FD) {
			found = 1;
		} else {
			file = file->next;
		}
	} while (file != NULL && !found);

	if (found) {
		//printf("FOUND write file\n");
		//get the inode to get address
		readBlock(diskNum, FD, block);  //block = inode
		//SET MODIFIED TIME HERE
		tm_info = localtime(&timer);
		strftime(block + 45, 26, "%H:%M:%S", tm_info);

		if (block[13]) {
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
					memcpy(block + 4, ind * (BLOCKSIZE - 4) + buffer, BLOCKSIZE - 4);
					writeBlock(diskNum, freeind, block);
					freeind = nextind;
				}


			}
		} else
			status = MODIFYING_READ_ONLY_FILE;
	} else
		status = FILE_NOT_OPENED;

	free(block);

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
	time(&timer);
	//make sure the file is open

	if(fileList == NULL)
		status = FILE_NOT_OPENED;

	do {
		if (file->fd == FD) {
			found = 1;
		} else {
			file = file->next;
		}
	} while (file != NULL && !found);

	if (!found) 
		return FILE_DOES_NOT_EXIST;//set error...figure out details: TODO

	//file holds filepointer...maybe move to inode?  oh well
	//get which file extent:
	int ext = file->filepointer / (BLOCKSIZE - 4); //which extent to go to
	int extind = (file->filepointer - (BLOCKSIZE - 4) * ext) + 4;  //index into the correct extent
	readBlock(diskNum, FD, block);  //block = inode

	//SET ACCESS TIME
	tm_info = localtime(&timer);
	strftime(block + 71, 26, "%H:%M:%S", tm_info);
	writeBlock(diskNum, FD, block);

	int z;
	memcpy(&z, block + 15, 4);
	if (z <= file->filepointer)  
		status = READING_BEYOND_END_OF_FILE;
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

	free(block);

	return status;
}

/*
 *
 */
int setFreeBlock(int bnum) {
	int freeBlockIndex = -1;
	char *superBlock = calloc(sizeof(char), BLOCKSIZE);
	char *newFree = calloc(sizeof(char), BLOCKSIZE);
	int freeBlock;
	
	readBlock(diskNum, 0, superBlock); //Read in superblock
	readBlock(diskNum, bnum, newFree);

	newFree[2] = superBlock[4];  //new free block points to what superblock used to
	superBlock[4] = bnum; //superbloc points to new block
	newFree[0] = (char) 4;  //set type of block
	writeBlock(diskNum, bnum, newFree);
	writeBlock(diskNum, 0, superBlock);	
	
	free(superBlock);
	free(newFree);
	
	return freeBlock; //We return the absolute index of the freeblock
}

/*
 * Deletes a file and marks its blocks as free on a disk.
*/
int tfs_deleteFile(fileDescriptor FD) {
	int prevfile = 1; //root inode
	char *inode = calloc(sizeof(char), BLOCKSIZE);
	char *extent = calloc(sizeof(char), BLOCKSIZE);
	char *rootinode = calloc(sizeof(char), BLOCKSIZE);
	char *previnode = calloc(sizeof(char), BLOCKSIZE);
	int status = -25; //File not found!
	int found = 0;

	readBlock(diskNum, 1, inode);
	int ind;
	if (inode[2] == FD) //root inode
		found = 1;
	while ((ind = inode[2]) != -1 && !found) {
		readBlock(diskNum, ind, inode);
		if (inode[2] == FD) {
			found = 1;
			prevfile = ind;
		}
	}

	readBlock(diskNum, FD, inode);
	if (inode[0] != 2) {
		found = 0;
	}

	if (found) {
		//tfs_closeFile(FD);
		status = 0;

		//set file extents free (updates superblock pointer too)
		if (inode[13]) {
			tfs_closeFile(FD);
			int extind = inode[14];

			do {
				readBlock(diskNum, extind, extent);
				int temp = extind; //index of extent to free
				extind = extent[2]; //save next location of extent
				setFreeBlock(temp);
			} while (extind != -1);

			//set inode free (update inode linked list (assumes that linked list matches actual list order...))
			readBlock(diskNum, 1, rootinode);
			if (prevfile == 1) { //first one is file in list
				rootinode[2] = inode[2]; //point superblock to what inode used to point to
				setFreeBlock(FD); //free inode
				writeBlock(diskNum, 1, rootinode);
				//update dynamically allocated linked list
				/*if (fileList->next == NULL) 
					fileList = NULL;
				else
					fileList = file->next;*/
			} else { //file is in middle or end...
				readBlock(diskNum, prevfile, previnode);
				previnode[2] = inode[2];
				setFreeBlock(FD);
				writeBlock(diskNum, prevfile, previnode);
				/*prevfile->next = file->next;*/
			}
		} else
				status = MODIFYING_READ_ONLY_FILE; //tried to modify ro file
	} else {
		status = FILE_NOT_FOUND;
	}

	free(extent);
	free(inode);
	free(rootinode);
	free(previnode);

	return status;
}

//ERROR codes
//STACK OVERFLOW STERNNO base code
typedef struct {
  int errorCode;
  const char* errorString;
} errorType;

errorType errs[] = {
  {FILESYSTEM_NOT_FOUND_OR_OF_SIZE_ZERO, "FILESYSTEM_NOT_FOUND_OR_OF_SIZE_ZERO"},
  {MISALIGNED_BLOCKSIZE, 				 "MISALIGNED_BLOCKSIZE"},
  {READ_FAILURE,						 "READ_FAILURE"},
  {WRITE_ERROR,							 "WRITE_ERROR"},
  {FILESYSTEM_TOO_SMALL,				 "FILESYSTEM_TOO_SMALL"},
  {BAD_FILESYSTEM_FORMAT,				 "BAD_FILESYSTEM_FORMAT"},
  {NO_FILESYSTEM_MOUNTED,				 "NO_FILESYSTEM_MOUNTED"},
  {BAD_FILENAME_LENGTH,					 "BAD_FILENAME_LENGTH"},
  {FILESYSTEM_IS_FULL,					 "FILESYSTEM_IS_FULL"},
  {NO_FILES_IN_FILESYSTEM,				 "NO_FILES_IN_FILESYSTEM"},
  {EMPTY_FILE,							 "EMPTY_FILE"},
  {FILE_DOES_NOT_EXIST,					 "FILE_DOES_NOT_EXIST"},
  {EXCESS_WRITE_SIZE,					 "EXCESS_WRITE_SIZE"},
  {MODIFYING_READ_ONLY_FILE,			 "MODIFYING_READ_ONLY_FILE"},
  {FILE_NOT_OPENED,						 "FILE_NOT_OPENED"},
  {READING_BEYOND_END_OF_FILE,			 "READING_BEYOND_END_OF_FILE"},
  {FILENAME_TOO_LONG,					 "FILENAME_TOO_LONG"},
  {FILE_NOT_FOUND,						 "FILE_NOT_FOUND"},
  {0,           "NULL"           },
};

int errorStr(int errorValue) {
  int i = 0;
  int found = 0;
  while (errs[i].errorCode != 0) {
    if(errs[i].errorCode == errorValue && errorValue != 0) {
      //Found the correct error index value
      found = 1;
      break;
    }
    i++;
  }
  if(found) {
    printf("Error number: %d (%s)\n",errs[i].errorCode, errs[i].errorString);
  } else {
    printf("No Error\n");
  }
  if(found) {
    return i;
  } else {
    return -1;
  }
}