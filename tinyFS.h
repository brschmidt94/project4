#define BLOCKSIZE 256
#define DEFAULT_BLOCK_SIZE 10240
#define DEFAULT_DISK_NAME "tinyFSDisk"

typedef int fileDescriptor;

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
void tfs_readFileInfo(fileDescriptor FD);
int tfs_makeRO(char *name);
int tfs_makeRW(char *name);
int tfs_writeByte(fileDescriptor FD, unsigned int data);
int tfs_rename(char *name, char *newName);
