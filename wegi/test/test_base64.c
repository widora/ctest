/*-------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Test egi_xxx_base64() functions.

Midas Zhou
-------------------------------------------------------------------*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "egi_utils.h"

int main(void)
{
	FILE *fp=NULL;
	int fd;
	struct stat sb;
	size_t fsize;
	unsigned char *fmap;
	char *buff;
	int ret=0;

	fd=open("/mmc/webcam.jpg",O_RDONLY);
	if(fd<0) {
		perror("open");
		return -1;
	}
	/* get size */
	if( fstat(fd,&sb)<0) {
		perror("fstat");
		return -2;
	}
	fsize=sb.st_size;


	/* allocate buff */
	buff=calloc((fsize+2)/3+1, 4); /* 3byte to 4 byte, 1 more*/
	if(buff==NULL) {
		printf("Fail to calloc buff\n");
		fclose(fp);
		return -3;
	}

        /* mmap file */
        fmap=mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
        if(fmap==MAP_FAILED) {
                perror("mmap");
                return -2;
        }

	ret=egi_encode_base64(fmap, fsize, buff);
	printf("ret=%d, buff= %s\n",ret,buff);


	munmap(fmap,fsize);
	close(fd);
	free(buff);

	return 0;
}
