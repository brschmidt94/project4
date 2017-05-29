#include <stdio.h>
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