#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "egi_https.h"
#include "egi_utils.h"
#include "egi_cstring.h"

static char buff[32*1024];      /* for curl returned data */

//char* cstr_parse_html_tag(const char* str_html, const char *tag, char **content, int *length);
static size_t curlget_callback(void *ptr, size_t size, size_t nmemb, void *userp);

const char *strhtml="<p> dfsdf sdfig df </p>";
//const char *strRequest="http://mini.eastday.com/mobile/191123112758979.html";
const char *strRequest="http://mini.eastday.com/mobile/191123190527243.html";

int main(void)
{
	char *content=NULL;
	int len;
	char *pstr=NULL;

	printf("%s\n", strhtml);
	pstr=cstr_parse_html_tag(strhtml, "p", &content, &len);
	printf("pstr: %s\n",pstr);
	printf("Get %d bytes content: %s\n", len, content);
	egi_free_char(&content);

	/* clear buff */
	memset(buff,0,sizeof(buff));
        /* Https GET request */
        if(https_curl_request(strRequest, buff, NULL, curlget_callback)!=0) {
                 printf("Fail to call https_curl_request()!\n");
                 exit(-1);
         }
	printf("%s\n", buff);

	/* Parse HTML */
	pstr=buff;
	do {
		/* parse returned data as html */
		pstr=cstr_parse_html_tag(pstr, "p", &content, &len);
		if(content!=NULL)
			printf("%s\n",content);
		egi_free_char(&content);
		//printf("Get %d bytes content: %s\n", len, content);
	} while( pstr!=NULL );

	egi_free_char(&content);
	return 0;
}

/*-----------------------------------------------
 A callback function to deal with replied data.
------------------------------------------------*/
static size_t curlget_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
        strcat(userp, ptr);
        return size*nmemb;
}


#if 0  /////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------------------
Get pointer to the beginning a HTML element content, which is defined between start tag
and end tag. It returns only the first matched case!

Note:
1. Input tag MUST be closed type, it appeares as <X> ..... </X> in HTML string.
   Void elements, such as <hr />, <br /> are not applicable for the function!
   If input html string contains only single <X> OR </X> tag, a NULL pointer will
   be returned.
2. The parameter *content has independent memory space if it is passed to the caller,
   so the caller is responsible to free it later.
3. The returned char pointer is a reference pointer to a position in original HTML string,
   so it needs NOT to be freed.

4. Limits:
   4.1 Length of tag.

@str_html:	Pointer to a html string.
@tag:		Tag name
		Example: "p" as paragraph tag
			 "h1","h2".. as heading tag
@len:		Pointer to pass length of the element content, in bytes.
		if NULL, ignore.
@content:	Pointer to pass element content.
		if NULL, ignore.
		!!! --- Note: the content has independent memory space, and
		so do not forget to free it. --- !!!

Return:
	Pointer to taged content in str_html:		Ok
	NULL:						Fails
--------------------------------------------------------------------------------------*/
char* cstr_parse_html_tag(const char* str_html, const char *tag, char **content, int *length)
{
	char stag[16]; 	/* start tag */
	char etag[16]; 	/* end tag   */
	char *pst=NULL;	/* Pointer to the beginning of start tags in str_html
			 * then adjusted to the beginning of content later.
			 */
	char *pet=NULL; /* Pointer to the beginning of end tag in str_html */
	int  len=0;	/* length of content, in bytes */
	char *pctent=NULL; /* allocated mem to hold copied content */

	/* check input data */
	if( strlen(tag) > 16-4 )
		return NULL;
	if(str_html==NULL || tag==NULL )
		return NULL;

	/* init. start/end tag */
	memset(stag,0, sizeof(stag));
	strcat(stag,"<");
	strcat(stag,tag);
	//printf("stag: %s\n", stag);

	memset(etag,0, sizeof(etag));
	strcat(etag,"</");
	strcat(etag,tag);
	strcat(etag,">");
	//printf("etag: %s\n", etag);

	/* locate start and end tag in html string */
	pst=strstr(str_html,stag);
	if(pst != NULL)			/* get end of start tag */
		pst=strstr(pst,">");

	pet=strstr(pst,etag);

	/* Only if tag content is valid/available:  Copy and pass parameters */
	if( pst!=NULL && pet!=NULL ) {
		/* get length of content */
		pst += strlen(">");	/* skip '>', move to the beginning of the content */
		len=pet-pst;

		/* 1. Calloc pctent and copy content */
		if( content != NULL) {
			pctent=calloc(1, len+1);
			if(pctent==NULL)
				printf("%s: Fail to calloc pctent...\n",__func__);
			else
				/* Now pst is pointer to the beginning of the content */
				strncpy(pctent,pst,len);
		}
	}

	/* pass to the caller anyway */
	if( content != NULL )
		*content=pctent;
	if( length !=NULL )
		*length=len;

	/* Now pst is pointer to the beginning of the content */
	return pst;
}
#endif ////////////////////////////////////////////////////////////////////////
