#include <stdlib.h>
#include "egi.h"
#include "egi_list.h"
#include "egi_objlist.h"
#include "egi_symbol.h"
#include "egi_color.h"

#if 0
EGI_LIST *egi_list_new (
        int x0, int y0, /* left top point */
        int inum,       /* item number of a list */
        int width,      /* h/w for each list item - ebox */
        int height,
        int frame,      /* -1 no frame for ebox, 0 simple .. */
        int nl,         /* number of txt lines for each txt ebox */
        int llen,       /* in byte, length for each txt line */
        struct symbol_page *font, /* txt font */
        int txtoffx,     /* offset of txt from the ebox, all the same */
        int txtoffy,
        int iconoffx,   /* offset of icon from the ebox, all the same */
        int iconoffy
);

int egi_list_free(EGI_LIST *list);
int egi_list_activate(EGI_LIST *list);
int egi_list_refresh(EGI_LIST *list);
int egi_list_updateitem(EGI_LIST *list, int n, uint16_t prmcolor, char **data);

#endif

void egi_list_test(void)
{
	int i,j;
	int inum=5;
	int nl=2;

	/* 1. create a list */
	EGI_LIST *list=egi_list_new (
        	0,0,  	//int x0, int y0, /* left top point */
	        inum,	//int inum,       /* item number of a list */
	       	240,	//int width,      /* h/w for each list item - ebox */
	        60,	//int height,
	        0,      //frame, 	  /* -1 no frame for ebox, 0 simple .. */
	        2,	//int nl,         /* number of txt lines for each txt ebox */
	        30,	//int llen,       /* in byte, length for each txt line */
	        &sympg_testfont,	//struct symbol_page *font, /* txt font */
	        30,	//int txtoffx,     /* offset of txt from the ebox, all the same */
	        4,	//int txtoffy,
	        0,	//int iconoffx,   /* offset of icon from the ebox, all the same */
	        0	//int iconoffy
	);
printf("egi_list_test(): finish egi_list_new(). \n");

	/* 2. icons to be loaded later */



	/* 3. update item */
	char data[5][2][30] =  /* inum=5; nl=2; llen=30 */
	{
		{"Widora-NEO","mt7688"},
		{"Widora-BIT5","mt7628dan"},
		{"Widora-AIR","ESP32"},
		{"Widora-BIT3","mt7688AN"},
		{"Widora-Ting","SX1278"}
	};
	char **pdata;
	pdata=malloc(2*sizeof(char *));
	for(i=0; i<inum; i++)
	{
		for(j=0;j<nl;j++)
		{
			pdata[j]=&data[i][j][0];
			printf("pdata[%d]: %s \n",j,pdata[j]);
		}

		egi_list_updateitem(list, i, egi_color_random(light), pdata);
	}

	/* 4. activate the list	*/
	egi_list_activate(list);
	printf("egi_list_test(): finish egi_list_activate(). \n");
}


