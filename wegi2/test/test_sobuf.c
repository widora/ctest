/*---------------------------------------------------------------------
A simple and interesting test for stdout buffer setting.


Note:
0. stdout and stdin are both line buffered as default.
1. void setbuf(FILE *stream, char *buf)
   	== setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ)

   Note: So if buf is NOT NULL, it's fully buffered actually, and it need
         to call fflush() to send content to terminal if want not delay.

2. void setbuffer(FILE *stream, char *buf, size_t size)
	== setvbuf(stream, buf, buf ? _IOFBF : _IONBF, size)

3. void setlinebuf(FILE *stream);
	== setvbuf(stream, NULL, _IOLBF, 0)

4. tcsetattr():
   4.1 If disable canonical mode, as without buffer:
       4.1.1 This will also disable input_line editting!
       4.1.2  Then Ctrl+D will return 0x04(EOT)
   4.2 If enable canonical mode, as with buffer:
       4.2.1 Then Ctrl+D will return endless 0xFF as EOF;

5. tcsetattr(): If set MIN (c_cc[VMIN]) and TIME (c_cc[VTIME]) both 0,
   then getchar() will return 0(EOF) immediately!


Midas Zhou
midaszhou@yahoo.com
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int k;
	FILE *pfil;
	size_t nread;
	static char buf[BUFSIZ];
	char ch;
	char shcmd[256]={0}; 	/* Input shell command string */



#if  0 ///////////////////////////////////
  // echo $USER@$HOSTNAME:$PWD#
	//pfil=popen("ls /mmc","r");
	pfil=popen(argv[1],"r");

   do {
	bzero(buf,sizeof(buf));
	nread=fread(buf, 1, BUFSIZ, pfil);
	//fgets(buf, sizeof(buf), pfil);
	printf("nread=%d\n",nread);
	printf("%s", buf);
   } while(nread==sizeof(buf));

	pclose(pfil);
	return 0;
#endif //////////////////////////////////

	int i;

        /* Setup with new settings */
        struct termios new_termioset;
        //new_termioset=old_termioset;
	tcgetattr(0, &new_termioset);
        //new_termioset.c_lflag |= ICANON;      /* enable canonical mode, with buffer */
        new_termioset.c_lflag &= ~ICANON;      /* disable canonical mode, without buffer, But this will disable inputline editting */
        new_termioset.c_lflag |= ECHO;        /* enable echo */
        //new_termioset.c_lflag &= ~ECHO;        /* disable echo */
	/* If Set 0,0 as polling type, getchar() will get EOF immediately! */
        new_termioset.c_cc[VMIN]=1;
        new_termioset.c_cc[VTIME]=0;
        /* Set parameters to the terminal. TCSANOW -- the change occurs immediately.*/
        if( tcsetattr(0, TCSANOW, &new_termioset)<0 )
                printf("%s: Fail tcsetattr, Err'%s'\n", __func__, strerror(errno));

	setbuf(stdout, NULL);    /* Change to non_buffered */
	//setbuf(stdout, buf);   /* Fully buffered */

 #if 0 /* ----- TEST:  Ctrl+D will return: 1. NON_ICANON mode: 0x04(EOT) 2. ICANON mode: endless 0xFF(EOF) */
	while( ch=getchar() ) {   /* OR use read(STDIN_FILENO, &ch, 1) */
		printf("ch=0x%02X\n",(unsigned char)ch);
	}
	exit(0);
 #endif
	printf("Input your Shell command: ");
	fflush(stdout);
	k=0;
	while( (ch=getchar())!=0 ) {	/* Ctrl+D will trigger endless 0xFF input */
		//putchar(ch); /* Not necessary, as ECHO in on for TCATTR */

		/* Default user stdout buffer is block_buffered?  Must control fflush by the USER! */
		//fflush(stdout);

		/* Enter a complete line */
		if(ch=='\n') {
			/* !! K MAY overflows */
			shcmd[k]=ch;
			//printf("Shcmd: %s\n", shcmd);
			system(shcmd);
			k=0;
			bzero(shcmd,sizeof(shcmd));
			printf("Input your Shell command: ");
			//fflush(stdout);
		}
		/* Accumulate input chars */
		else {
			shcmd[k++]=ch;
			if(k>=sizeof(shcmd)) {
				printf("Shell command too long!\n");
				k=0;
				bzero(shcmd,sizeof(shcmd));
				printf("Input your Shell command: ");
				fflush(stdout);
			}
		}
	}

	if(ch==EOF)
		printf("get EOF! quit now...\n");
	printf("--- END ---\n");
	printf("User buffer content:\n");
	printf("%s\n",buf);

	//fflush(stdout);

	return 0;
}

