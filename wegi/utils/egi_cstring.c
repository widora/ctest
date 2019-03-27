/*------------------------------------------------
Char and String Functions

Midas Zhou
------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "egi_cstring.h"
#include "egi_log.h"

/*--------------------------------------------------
Trim all spaces at end of a string, return a pointer
to the first non_space char.
Return:
	pointer	to a char	OK, spaces trimed.
        NULL			Input buf invalid
--------------------------------------------------*/
char * cstr_trim_space(char *buf)
{
	char *ps, *pe;

	if(buf==NULL)return NULL;

	/* skip front spaces */
	ps=buf;
	for(ps=buf; *ps==' '; ps++)
	{ };

	/* eat up back spaces/returns, replace with 0 */
	for(  pe=buf+strlen(buf)-1;
	      *pe==' '|| *pe=='\n' || *pe=='\r' ;
	      (*pe=0, pe-- ) )
	{ };

	/* if all spaces, or end token
	 * if ps==pe, means there is only ONE char in the line.
	*/
	if( (long)ps > (long)pe ) { //ps==pe || *ps=='\0' ) {
//		printf("%s: Input line buff contains all but spaces. ps=%p, pe=%p \n",
//					__func__,ps,pe);
			return NULL;
	}

	//printf("%s: %s, [0]:%c, [9]:%c, [10]:%c\n",__func__,ps,ps[0],ps[9],ps[10]);
	return ps;
}

/*----------------------------------------------------------------------------------
Search given SECTION and KEY string in the config file, copy VALUE
string to the char *value if found.

sect:	Char pointer to a given SECTION name.
key:	Char pointer to a given KEY name.
pvalue:	Char pointer to a char buff that will receive found VALUE string.

NOTE:
1. A config file should be edited like this:
# comment comment comment
# comment comment commnet
        # comment comment
  [ SECTION1]
KEY1 = VALUE1
KEY2= VALUE2

##########   comment
	####comment
##
[SECTION2 ]
KEY1=VALUE1
KEY2= VALUE2
...

1. Lines starting with '#' are deemed as comment lines.
2. Lines starting wiht '[' are deemed as start/end/boundary of a SECTION.
3. Non_comments lines containing a '=' are parsed as assignment for KEYs with VALUEs.
4. All spaces beside SECTION/KEY/VALUE strings will be ignored/trimmed.
5. If there are more than one section with the same name, only the first
   one is valid, and others will be all neglected.

		[[ ------  LIMITS -----  ]]
6. Max. length for one line in a config file is 256-1. ( see. line_buff[256] )
7. Max. length of a SECTION/KEY/VALUE string is 64-1. ( see. str_test[64] )

Return:
	3	VALE string is NULL
	2	Fail to find KEY string
	1	Fail to find SECTION string
	0	OK
	<0	Fails
------------------------------------------------------------------------------------*/
int egi_get_config_value(char *sect, char *key, char* pvalue)
{
	int lnum=0;
	int ret=0;

	FILE *fil;
	char line_buff[256];
	char *ps=NULL, *pe=NULL; /* start/end char pointer for [ and  ] */
	char *pt=NULL;
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
	//printf("Succeed to open '%s', with file descriptor %d. \n",EGI_CONFIG_PATH, fileno(fil));

	fseek(fil,0,SEEK_SET);

	/* Search SECTION and KEY line by line */
	while(!feof(fil))
	{
		lnum++;

		memset(line_buff,0,sizeof(line_buff));
		fgets(line_buff,sizeof(line_buff),fil);
		//printf("line_buff: %s\n",line_buff);

		/* 0. cut the return key '\r' '\n' at end .*/
		/*   OK, cstr_trim_space() will do it */

		/* 1.  Search SECTION name in the line_buff */
		if(!found_sect)
		{
			/* Ignore comment lines starting with '#' */
			ps=cstr_trim_space(line_buff);
			/* get rid of all_space/empty line */
			if(ps==NULL) {
				printf("config file:: Line:%d is an empty line!\n", lnum);
				continue;
			}
			else if( *ps=='#') {
//				printf("Comment: %s\n",line_buff);
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
					printf("Found SECTION:[%s] \n",str_test);
					found_sect=1;
				}
			}
		}
		/* 2. Search KEY name in the line_buff */
		else /* IF found_sect */
		{
			ps=cstr_trim_space(line_buff);
			/* bypass blank line */
			if( ps==NULL )continue;
			/* bypass comment line */
			else if( *ps=='#' ) {
//				printf("Comment: %s\n",line_buff);
				continue;
			}
			/* If first char is '[', it reaches the next SECTION, so give up. */
			else if( *ps=='[' ) {
				printf("Get bottom of the section, fail to find key '%s'.\n",key);
				ret=2;
				break;
			}

			/* find first '=' */
			ps=strstr(line_buff,"=");
			/* assure NOT null and '=' is NOT a starting char 
			 * But, we can not exclude spaces before '='.
			 */
			if( ps!=NULL && ps != line_buff ) {
				memset(str_test,0,sizeof(str_test));
				/* get key name string */
				strncpy(str_test, line_buff, ps-line_buff);
//				printf(" key str_test: %s\n",str_test);
				/* assure key name is not NULL */
				if( (ps=cstr_trim_space(str_test))==NULL) {
				   printf("Key name is NULL in line_buff: '%s' \n",line_buff);
				   continue;
				}
				/* if key name matches */
				else if( strcmp(key,ps)==0 ) {
					//printf("found KEY:%s \n",str_test);
					ps=strstr(line_buff,"="); /* !!! again, point ps to '=' */
					pt=cstr_trim_space(ps+1);
					/* Assure the value is NOT null */
					if(pt==NULL) {
					   printf("%s: Value of key [%s] is NULL in line_buff: '%s' \n",
										__func__, key, line_buff);
					   ret=3;
					   break;
					}
					/* pass VALUE to pavlue */
					else {
					   strcpy(pvalue, pt);
					   printf("%s: Found  Key:[%s],  Value:[%s] \n",
										__func__, key,pvalue);
					   ret=0; /* OK, get it! */
					   break;
					}
				}
			}
		}

	} /* end of while() */

	if(!found_sect) {
		printf("%s: Fail to find given SECTION:[%s] in config file.\n",__func__,sect);
		ret=1;
	}
#if 1
	/* log errors */
	if(ret !=0 ) {
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to get value of key:[%s] in section:[%s] in config file.\n",
										      __func__, key, sect);
	}
	else {
		EGI_PLOG(LOGLV_CRITICAL,"%s: Get value of key:[%s] in section:[%s] in config file.\n",
										      __func__, key, sect);
	}
#endif
	fclose(fil);
	return ret;
}
