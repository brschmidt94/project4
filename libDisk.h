extern int openDisk(char *, int);
extern int readBlock(int, int, void *);
extern int writeBlock(int, int, void *);
extern void closeDisk(int);