/*----------------------------------
   use dmesg to check more result details
-----------------------------------*/
#include <stdio.h>
#include <fcntl.h>	/*open */
#include <unistd.h> 	/*write,read,close */
#include <sys/ioctl.h>  /* ioctl */
#include <sys/mman.h> 	/* mmap */
#include <string.h>

/*  ioctl definition */
#define HELLO_IOC_MAGIC 'h'
#define HELLO_IOCCMD_READ  _IOR(HELLO_IOC_MAGIC,1,int) //_IOR(type,nr,size)
#define HELLO_IOCCMD_WRITE _IOW(HELLO_IOC_MAGIC,2,int)  //_IOW(type,nr,size)
#define HELLO_IOC_MAXNR 2

#define PAGE_SIZE 4096

int main(void)
{
	int fd;
	int n,k;
	int dat;
	char *wbuf="abcdefghijklmnopqrst";
	char *wbuf2="12345678901234567890";
	char rbuf[100]={};
	char *path = "/dev/hello_dev";
	char *map;

	printf("-------- file op test -------\n");
	fd=open(path,O_RDWR);
	n=write(fd,wbuf,20); /* write first!!! or the reader will be put in wait_evet */
	printf("wbuf[]=%s\n",wbuf);
	printf("n=write(fd,wbuf,10)=%d\n",n);
	k=lseek(fd,10, SEEK_SET);
	printf("k=lseek(fd,10,SEEK_SET)=%d\n",k);
	n=read(fd,rbuf,20);
	printf("n=read(fd,rbuf,20)=%d, rbuf[]=%s\n",n,rbuf);

	printf("\n-------- ioctl test, tmp data write/read -------\n");
	ioctl(fd, HELLO_IOCCMD_READ,&dat);
	printf("ioctl read get dat=%d\n",dat);
	dat=54321;
	ioctl(fd, HELLO_IOCCMD_WRITE,&dat);
	printf("ioctl write dat=%d\n",dat);

	printf("\n-------- mmap test -------\n");
	/* pagesize = 4096 bytes */
	map=(char *)mmap(NULL,PAGE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(map == MAP_FAILED){
		perror("mmap");
	}
	else {
		printf("dev mem: %s\n",map);
		strcpy(map,"1234567890----------&&&&&&&&&");
		printf("dev mem after strcpy: %s\n",map);
		map[5]='*';
		printf("dev mem after [5]='*': %s\n",map);
	}

	/* hold for cheking stat and result */
	while(1)
	{}

	munmap(map,PAGE_SIZE);
	close(fd);
	return 0;
}

/*  ----- result ---

[34366.731380] --- HELLO_IOCCMD_READ from user_hello:  put_user() tmp=12345
[34366.731383] user_hello:Permission not allowed for the user to write!
[34366.731398] simple VMA open, virt 7f4ad4a31000, phys 0
[34449.137916] ptrace attach of "./user_hello"[10689] was attempted by "cat mem"[10717]

midas-zhou@midas-Gen8:/proc/10689$ sudo cat maps
00400000-00401000 r-xp 00000000 08:05 41953310                           /home/midas-zhou/kmods/ubuntu_kmods/hello/user_hello
00600000-00601000 r--p 00000000 08:05 41953310                           /home/midas-zhou/kmods/ubuntu_kmods/hello/user_hello
00601000-00602000 rw-p 00001000 08:05 41953310                           /home/midas-zhou/kmods/ubuntu_kmods/hello/user_hello
00a95000-00ab6000 rw-p 00000000 00:00 0                                  [heap]
7f4ad4443000-7f4ad4603000 r-xp 00000000 08:01 1053923                    /lib/x86_64-linux-gnu/libc-2.23.so
7f4ad4603000-7f4ad4803000 ---p 001c0000 08:01 1053923                    /lib/x86_64-linux-gnu/libc-2.23.so
7f4ad4803000-7f4ad4807000 r--p 001c0000 08:01 1053923                    /lib/x86_64-linux-gnu/libc-2.23.so
7f4ad4807000-7f4ad4809000 rw-p 001c4000 08:01 1053923                    /lib/x86_64-linux-gnu/libc-2.23.so
7f4ad4809000-7f4ad480d000 rw-p 00000000 00:00 0
7f4ad480d000-7f4ad4833000 r-xp 00000000 08:01 1053895                    /lib/x86_64-linux-gnu/ld-2.23.so
7f4ad4a0e000-7f4ad4a11000 rw-p 00000000 00:00 0
7f4ad4a31000-7f4ad4a32000 rw-s 00000000 00:06 555        !!!!!!!!       /dev/hello_dev  
7f4ad4a32000-7f4ad4a33000 r--p 00025000 08:01 1053895                    /lib/x86_64-linux-gnu/ld-2.23.so
7f4ad4a33000-7f4ad4a34000 rw-p 00026000 08:01 1053895                    /lib/x86_64-linux-gnu/ld-2.23.so
7f4ad4a34000-7f4ad4a35000 rw-p 00000000 00:00 0 
7ffcfb082000-7ffcfb0a3000 rw-p 00000000 00:00 0                          [stack]
7ffcfb1d1000-7ffcfb1d4000 r--p 00000000 00:00 0                          [vvar]
7ffcfb1d4000-7ffcfb1d6000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]


*/
