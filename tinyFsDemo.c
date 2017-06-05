#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libDisk.h"
#include "tinyFS.h"
#include "TinyFS_errno.h"

int main(int argc, char** argv) {
	tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_BLOCK_SIZE);

	return 0;
}