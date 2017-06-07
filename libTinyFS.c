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
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD, char *buffer, int size); //K *set errors for getFreeBlock? why (char)
//K *check todos: need diskNum saved in mount
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD, char *buffer); //K *need filepointer to be set to -1 in openFile
int tfs_seek(fileDescriptor FD, int offset);
void tfs_readdir();
int verifyFormat(int filename); //K *check logic
int getFreeBlock();
int findFile(char *name);
void printDiagnostics(int diskNum);

int tfs_makeRO(char *name);
int tfs_makeRW(char *name);
int tfs_writeByte(fileDescriptor FD, unsigned int data);

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
//TODO KEEP TRACK OF SIZES

time_t timer;
char time_buffer[26];
struct tm* tm_info;

int main(int argc, char** argv) {
	int disk = tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_BLOCK_SIZE);
	fileDescriptor fd;
	char *buffer =  calloc(BLOCKSIZE, sizeof(char));
	int i;
	for (i = 0; i < BLOCKSIZE; i++) {
		buffer[i] = i;
	}

	int mountcheck = tfs_mount(DEFAULT_DISK_NAME);

	if (mountcheck >=0) {
		
		fd = tfs_openFile("cats");
		tfs_writeFile(fd, buffer, 256);  //should take up 2 free blocks because extra bytes in beginning

		char *buf = calloc(1, sizeof(char));
		for (i = 0; i < 256; i++) {
			tfs_readByte(fd, buf);
			printf("%hhx ", buf[0]);
		}
		printf("\n");

		//tfs_deleteFile(fd);

		int fd2;
		char *buffer2 =  calloc(BLOCKSIZE, sizeof(char));
		for (i = 0; i < 5; i++) {
			buffer2[i] = 12;
		}
		fd2 = tfs_openFile("dogs");

		struct openFile *file = fileList;

		int wr = tfs_writeFile(fd2, buffer2, 5);
		for (i = 0; i < 5; i++) {
			tfs_readByte(fd2, buf);
			printf("%hhx ", buf[0]);
		}
		tfs_deleteFile(fd2);

		printf("\n");
		//TODO: deleting multiple files doesn't work...also, open file twice...
		//tfs_deleteFile(fd2);

		//tfs_deleteFile(fd);

		tfs_rename("cats", "meow");

		fd2 = tfs_openFile("stuff");
		tfs_makeRO("stuff");
		if (tfs_writeFile(fd2, buffer2, 5) == -20) {
			printf("ERROR: TRIED WRITING TO RO\n");
		}
		if (tfs_deleteFile(fd2) == -20)
			printf("ERROR: TRIED DELETING RO\n");
		printDiagnostics(diskNum);
		tfs_readdir();
		tfs_unmount();
	} else 
		printf("bad mount\n");

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
			printf("File Extents List Address: %d\n", data[2]); //ADDED
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

//GOOD (except for the size saving?)
int tfs_mkfs(char *filename, int nBytes) {
	int index;
	int block;
	char *format;
	diskSize = nBytes; //Save size to instance variable
	
	time(&timer);
	
	if((diskNum = openDisk(filename, nBytes)) == -1)
		diskNum = -6; ////ERROR: Tried to make empty filesystem of size 0
	
	if(nBytes < 2) //We assume that a file of just superblock and root inode can be made
		diskNum = -4; //ERROR: FILESYSTEM SIZE TOO SMALL
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

		if(verifyFormat(status) != 0) {
			closeDisk(status);
			status = -8; //ERROR: TRIED TO MOUNT FILE WITH WRONG FORMAT
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

 	if (!mounted) {
 		//TODO: set errno /status
 		status = -1;
	}
	else {
 		closeDisk(diskNum);
 		mounted = 0;
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
			
			//SET PERMS		
			//////////////////////////////////
			block[13] = (char) 1; //Permissions byte RW
			/////////////////////////////////
			
			//Zero out data from before
			for(index = 15; index < 19; index++)
				block[index] = (char) 0;
			
			tm_info = localtime(&timer);
			strftime(block + 19, 26, "%H:%M:%S", tm_info);
			
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

//change the inode pointer to point to the offset instead of the first file extent
int tfs_seek(fileDescriptor FD, int offset) {
	struct openFile *file = fileList;
	int status = -25; //File not found!
	int foundFile = 0;
	
	if(file == NULL)
		status = -8; //ERROR: File empty on seek()
	else {
		while(!foundFile && file) {
			if(file->fd == FD) {
				foundFile = 1;
				status = 0;
				
				file->filepointer += offset;
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
	int status = -1;
	
	readBlock(diskNum, 1, block);
	
	if(!(strlen(newName) > 0 && strlen(newName) <= 8))
		return -36; //ERROR: filename of improper length
	
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
				status = -12; //ERROR: File does not exist!
			}
		}
	}
	
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
			//printf("%s - %d bytes\n", block + 15, block[4]);
			//CHANGED
			// get size of file in bytes
			int z;
			memcpy(&z, block +15, 4);
			printf("%s - %d bytes\n", block + 4, z);
			printf("Created: %s\n", block + 19);


			if(block[2] == -1)
				done = 1;
			else
				readBlock(diskNum, block[2], block);
		}
	}	
}





void tfs_readFileInfo(fileDescriptor FD) {
	char *block = (char *) calloc(sizeof(char), BLOCKSIZE);
	struct openFile *file = fileList;
	int foundFile = 1;
	int status = 0;
	
	if(file == NULL)
		status = -8; //ERROR: File empty on seek()
	else {
		while(!foundFile && file) {
			if(file->fd == FD) {
				foundFile = 1;
					
				readBlock(diskNum, file->fd, block);
				printf("%s - %d bytes\n", block + 15, block[4]);
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
	int status = -1;
	
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
				status = -12; //ERROR: File does not exist!
			}
		}
	}
	return status;
}

int tfs_makeRW(char *name) {
	int status = 0;

	return status;
}

int tfs_writeByte(fileDescriptor FD, unsigned int data) {
	int status = 0;

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
	if (validRead < 0 || buff[1] != 0x45)  //if reading superblock  doesn't cause error
		ret = -1;

	return ret;
}
		//save index of free blocks
		/*int freeind = buff[4];

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
		ret = -1;*/
	


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
	} while (file != NULL && !found);

	if (found) {
		//printf("FOUND write file\n");
		//get the inode to get address
		readBlock(diskNum, FD, block);  //block = inode
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
					memcpy(block + 4, ind *(BLOCKSIZE - 4) + buffer, BLOCKSIZE - 4);
					writeBlock(diskNum, freeind, block);
					freeind = nextind;
				}

				
			//SET MODIFIED TIME HERE


			}
		} else {
			status = -20; //TRIED TO MODIFY RO FILE
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
	} while (file != NULL && !found);

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
		
		
		
		
		
		
		// SET ACCESS TIME HERE
		
		
		
		
		
		
		
		
		
		
		
		
		//read in correct extent
		readBlock(diskNum, nextext, block);
		memcpy(buffer, block + extind, 1);
		file->filepointer++;
	}

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
	struct openFile *file = fileList;
	struct openFile *prevfile = NULL;
	char *inode = calloc(sizeof(char), BLOCKSIZE);
	char *extent = calloc(sizeof(char), BLOCKSIZE);
	char *rootinode = calloc(sizeof(char), BLOCKSIZE);
	char *previnode = calloc(sizeof(char), BLOCKSIZE);
	int status = -25; //File not found!
	int found = 0;
	
	if(file == NULL)
		status = -9; //ERROR: File empty on sdelete()

	//grab file from dynamically allocated linked list of open files
	do {
		if (file->fd == FD) {
			found = 1;
		} else {
			prevfile = file;
			file = file->next;
		}
	} while (file != NULL && !found);

	if (found) {
		status = 0;

		//set file extents free (updates superblock pointer too)
		readBlock(diskNum, FD, inode);
		if (inode[13]) {
			int extind = inode[14];

			do {
				readBlock(diskNum, extind, extent);
				int temp = extind; //index of extent to free
				extind = extent[2]; //save next location of extent
				setFreeBlock(temp);
			} while (extind != -1);

			//set inode free (update inode linked list (assumes that linked list matches actual list order...))
			readBlock(diskNum, 1, rootinode);
			if (prevfile == NULL) { //first one is file in list
				rootinode[2] = inode[2]; //point superblock to what inode used to point to
				setFreeBlock(FD); //free inode
				writeBlock(diskNum, 1, rootinode);
				//update dynamically allocated linked list
				if (fileList->next == NULL) 
					fileList == NULL;
				else
					fileList = file->next;
			} else { //file is in middle or end...
				readBlock(diskNum, prevfile->fd, previnode);
				previnode[2] = inode[2];
				setFreeBlock(FD);
				writeBlock(diskNum, prevfile->fd, previnode);
				prevfile->next = file->next;
			}
		} else {
			status = -20; //tried to modify ro file
		}
	}

	free(extent);
	free(inode);
	free(rootinode);
	free(previnode);

	return status;
}