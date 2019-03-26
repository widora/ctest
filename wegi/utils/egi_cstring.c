/*------------------------------------------------
Char and String Functions

Midas Zhou
------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "egi_cstring.h"


/*-----------------------------------------------
Trim spaces at back of a string, return a pointer
to the first non_space char.
Return:
	pointer	to a char	OK, spaces trimed.
        NULL			Input buf invalid
-----------------------------------------------*/
char * cstr_trim_space(char *buf)
{
	char *ps, *pe;

	if(buf==NULL)return NULL;

	/* eat up front spaces */
	ps=buf;
	for(ps=buf; *ps==' '; ps++)
	{};


	/* eat up back spaces */
	for(  pe=buf+strlen(buf)-1;
	      *pe==' ';
	      (*pe=0, pe-- ) )
	{ };

	/* if all spaces */
	if(ps==pe) {
		printf("%s: Input buff contains all but spaces.\n",__func__);
			return NULL;
	}

	//printf("%s: %s, [0]:%c, [9]:%c, [10]:%c\n",__func__,ps,ps[0],ps[9],ps[10]);
	return ps;
}

/*---------------------------------------------------------------------
Search given SECTION and KEY string in the config file, copy VALUE
string to the char *value if found.

sect:	Char pointer to a given SECTION name.
key:	Char pointer to a given KEY name.
pvalue:	Char pointer to a char buff that will receive found VALUE string.

NOTE:
1. A config file should be edited like this:
# comment comment comment
# comment comment commnet

[ SECTION1]
KEY1 = VALUE1
KEY2= VALUE2

[SECTION2 ]
KEY1=VALUE1
KEY2= VALUE2
...
2. All space will be ignored/trimmed in config file.
3. Max. Line Length of config file to be 256-1. ( see. line_buff[256] )
4. Max SECT/KEY/VALUE Length of config file to be 64-1. ( see. str_test[64] )

Return:
	>0	Not found
	0	OK
	<0	Fails
----------------------------------------------------------------------*/
int egi_get_config_value(char *sect, char *key, char* pvalue)
{
	int ret=1;

	FILE *fil;
//	char *sect="IOT_CLIENT";
//	char *key="server_ip";
	char line_buff[256];
	char *ps=NULL, *pe=NULL; /* start/end char pointer for [ and  ] */
	char str_test[64];
	int  found_sect=0; /* 0 - section not found, !0 - found */

	/* check input param */
	if(sect==NULL || key==NULL || pvalue==NULL) {
		printf("%s: One or more input param is NULL.\n",__func__);
		return -1;
	}

	/* open config file */
	fil=fopen( EGI_CONFIG_PATH, "r");
	if(fil==NULL) {
		printf("Fail to open config file '%s', %s\n",EGI_CONFIG_PATH, strerror(errno));
		return -2;
	}
	printf("Succeed to open '%s', with file descriptor %d. \n",EGI_CONFIG_PATH, fileno(fil));

	fseek(fil,0,SEEK_SET);

	/* Search SECTION and KEY line by line */
	while(!feof(fil))
	{
		memset(line_buff,0,sizeof(line_buff));
		fgets(line_buff,sizeof(line_buff),fil);
		//printf("line_buff: %s\n",line_buff);

		/* 1.  Search SECTION name in the line_buff */
		if(!found_sect)
		{
			/* Ignore comment lines which start with '#' */
			if( *cstr_trim_space(line_buff)=='#' ) {
				printf("Comment: %s\n",line_buff);
				continue;
			}
			/* search SECTION name between '[' and ']'
			 * Here we assume that [] only appears in a SECTION line, except comment line.
			 */
			ps=strstr(line_buff,"[");
			pe=strstr(line_buff,"]");
			if( ps!=NULL && pe!=NULL && pe>ps) {
				memset(str_test,0,sizeof(str_test));
				strncpy(str_test,ps+1,pe-ps+1-2); /* strip '[' and ']' */
				//printf("SECTION: %s\n",str_test);

				/* check if SECTION name matches */
				if( strcmp(sect,cstr_trim_space(str_test))==0 ) {
					printf("found SECTION:%s \n",str_test);

					found_sect=1;
				}
			}
		}
		/* 2. Search KEY name in the line_buff */
		else
		{
			/* find first '=' */
			ps=strstr(line_buff,"=");
			if( ps!=NULL && ps != line_buff ) {
				memset(str_test,0,sizeof(str_test));
				strncpy(str_test, ps, ps-line_buff);
				/* check if KEY name matches */
				if( strcmp(key,cstr_trim_space(str_test))==0 ) {
					//printf("found KEY:%s \n",str_test);
					/* then pass VALUE to pvalue */
					strcpy(pvalue, cstr_trim_space(ps+1));
					printf("found KEY:%s, VAULE:%s \n", key,pvalue);
					ret=0; /* OK, get it! */
					break;
				}
			}
		}

	} /* end of while() */

	fclose(fil);
	return ret;
}
