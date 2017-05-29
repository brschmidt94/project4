int openDisk(char *filename, int nBytes);
int readBlock(int disk, int bNum, void *block);
int writeBlock(intdisk, int bNum, void *block);
void closeDisk(int disk);