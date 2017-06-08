#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"


int main(int argc, char** argv) {
	//BASIC FUNCTIONALITY
	printf("Testing:\n\n");
	int disk = tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_BLOCK_SIZE);
	int disk2 = tfs_mkfs("FS2", 1024);
	printf("Making FS1 (default name and size (10,240))\n");
	printf("Making FS2 (named 'FS2' and size 1024)\n");

	fileDescriptor fd;

	char *buffer =  calloc(300, sizeof(char));
	int i;
	for (i = 0; i < 300; i++) {
		buffer[i] = i;
	}

	printf("\nMOUNTING FS1\n");
	printf("Opening and writing file: \"cats\" of 300 bytes (0-300 in hex)\n");
	printf("Write hex BEAD to first four bytes using writeByte()\n");
	printf("seek() back to beginning of file.\n");
	printf("Cause a 10 second pause and call readByte() 300 times.\n");
	printf("Bytes should read bead and then 4 to ff before it loops back to 0 through 2b:\n");
	int mountcheck = tfs_mount(DEFAULT_DISK_NAME);

	if (mountcheck >= 0) {
		fd = tfs_openFile("cats");
		errorStr(tfs_writeFile(fd, buffer, 300));  //should take up 2 free blocks because extra bytes in beginning

		char *buf = calloc(1, sizeof(char));
		tfs_writeByte(fd, 11);
		tfs_writeByte(fd, 0xE);
		tfs_writeByte(fd, 0xA);
		tfs_writeByte(fd, 0xD);
		tfs_seek(fd, 0);


		//delay by 1 min
    	unsigned int retTime = time(0) + 10;   // Get finishing time.
    	while (time(0) < retTime);               // Loop until it arrives.
		
		for (i = 0; i < 300; i++) {
			tfs_readByte(fd, buf);
			printf("%hhx ", buf[0]);
		}
		printf("\n\n");
		printf("ADDITIONAL FEATURE B (RENAME)\n");
		printf("Renaming cats to dogs.\n");
		tfs_rename("cats", "dogs");
		printf("Directories should be / and dogs.\n");
		printf("readdir() returns:\n");
		tfs_readdir();
		printf("\n");

		printf("ADDITIONAL FEATURE E (TIMESTAMPS)\n");
		printf("File info should show the same creation and modification time.\n");
		printf("The 300 bytes were read after the 10 second delay, so access time should differ.\n");
		printf("tfs_readFileInfo() returns:\n");
		tfs_readFileInfo(fd);

		printf("\nADDITIONAL FEATURE D (RO and WriteByte)\n");
		printf("write byte was shown above with writing bead\n");
		printf("Set dogs to read only and try writeFile(), deleteFile(), and writeByte():\n");
		tfs_makeRO("dogs");
		errorStr(tfs_writeFile(fd, buffer, 5));
		errorStr(tfs_deleteFile(fd));
		errorStr(tfs_writeByte(fd, 0xD));

		printf("UNMOUNT FS1\n");
		printf("\nMOUNT FS2\n");

		tfs_unmount();
		mountcheck = tfs_mount("FS2");
		int fd2;
		if (mountcheck >= 0) {
			printf("Try to make an empty file named...\"emptyfile\", which is too long:\n");
			errorStr(fd2 = tfs_openFile("emptyfile"));
			printf("Instead make an empty file named \"EMPTY\":\n");
			errorStr(fd2 = tfs_openFile("EMPTY"));
			printf("Directories should be / and EMPTY (of size 0).\n");
			printf("readdir() returns:\n");
			tfs_readdir();
			printf("\n");
		}

		printf("UNMOUNT FS2\n");
		tfs_unmount();
		printf("\nMOUNT FS1 AGAIN\nfiles should be as before\n\nreaddir() returns:\n");
		int mountcheck = tfs_mount(DEFAULT_DISK_NAME);
		if (mountcheck >= 0) {
			tfs_readdir();
			printf("\nMake 2 new files (\"almost\" and \"done\")\n");
			int fd3 = tfs_openFile("almost");
			int fd4 = tfs_openFile("done");
			printf("readdir() returns:\n");
			tfs_readdir();	
			printf("\nDelete \"almost\"\nreaddir() returns:\n");
			tfs_deleteFile(fd3);
			tfs_readdir();			
		}