#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tinyFS.h"
#include "TinyFS_errno.h"
#include "libTinyFS.h"

int main(int argc, char** argv) {
	int disk = tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_BLOCK_SIZE);
	fileDescriptor fd;

	char *buffer =  calloc(300, sizeof(char));
	int i;
	for (i = 0; i < 300; i++) {
		buffer[i] = i;
	}

	int mountcheck = tfs_mount(DEFAULT_DISK_NAME);

	if (mountcheck >= 0) {
		fd = tfs_openFile("cats");
		tfs_writeFile(fd, buffer, 300);  //should take up 2 free blocks because extra bytes in beginning

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
		printf("\n");

		//tfs_deleteFile(fd);
		//tfs_readFileInfo(fd);
		int fd2;
		char *buffer2 =  calloc(BLOCKSIZE, sizeof(char));
		for (i = 0; i < 5; i++) {
			buffer2[i] = 12;
		}
		fd2 = tfs_openFile("dogs");

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

		tfs_makeRW("stuff");
		if (tfs_writeFile(fd2, buffer2, 5) == 0) {
			printf("GOOD JOB WRITING TO RW\n");
		}

		//printDiagnostics(diskNum);
		tfs_readdir();
		tfs_readFileInfo(fd);
		tfs_unmount();
	} else 
		printf("bad mount\n");

	return 0;
}