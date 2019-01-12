#include <stdlib.h>
#include "egi.h"
#include "egi_list.h"
#include "egi_timer.h"
#include "egi_objlist.h"
#include "egi_symbol.h"
#include "egi_color.h"



int egi_list_test(EGI_EBOX *ebox, enum egi_btn_status status)
{
	int i,j;
	int inum=5;
	int nl=2;

	printf("egi_list_test(): egi_list_new()... \n");
	/* 1. create a list */
	EGI_LIST *list=egi_list_new (
        	0,30,  	//int x0, int y0, /* left top point */
	        inum,	//int inum,       /* item number of a list */
	       	240,	//int width,      /* h/w for each list item - ebox */
	        56,	//int height,
	        0,      //frame, 	  /* -1 no frame for ebox, 0 simple .. */
	        2,	//int nl,         /* number of txt lines for each txt ebox */
	        30,	//int llen,       /* in byte, length for each txt line */
	        &sympg_testfont,	//struct symbol_page *font, /* txt font */
	        45,	//int txtoffx,     /* offset of txt from the ebox, all the same */
	        2,	//int txtoffy,
	        0,	//int iconoffx,   /* offset of icon from the ebox, all the same */
	        0	//int iconoffy
	);
printf("egi_list_test(): finish egi_list_new(). \n");

	/* 2. icons to be loaded later */



	/* 3. update item */
	char data[5][2][30] =  /* inum=5; nl=2; llen=30 */
	{
		{"Widora-NEO     12345","mt7688"},
		{"Widora-BIT5     23233","mt7628dan"},
		{"Widora-AIR     43534","ESP32"},
		{"Widora-BIT     48433","mt7688AN"},
		{"Widora-Ting    496049999","SX1278"}
	};
	char **pdata;
	pdata=malloc(2*sizeof(char *));

	// 0xCFF9,0xFE73,0x07FF,0xFE73,0xFE7F
//	uint16_t color[]= {0xDEFB, 0xFE73, 0x07FF, 0xFE73, 0xFE7F};
	uint16_t color[]= {0x67F9, 0xFFE6, 0x0679, 0xFE79, 0x9E6C};

	for(i=0; i<inum; i++)
	{
		/* set data */
		for(j=0;j<nl;j++)
		{
			pdata[j]=&data[i][j][0];
			printf("pdata[%d]: %s \n",j,pdata[j]);
		}

		/* set icon */
		list->icons[i]=&sympg_icons;
		list->icon_code[i]=31;

		/* update items */
		egi_list_updateitem(list, i, color[i], pdata);
	}

	/* 4. activate the list	*/
	egi_list_activate(list);
	printf("egi_list_test(): finish egi_list_activate(). \n");

	tm_delayms(200);

	printf("egi_list_test(): start egi_list_free(list)... \n");
	egi_list_free(list);
	printf("egi_list_test(): start free(pdata)... \n");
	free(pdata);

	return 0;
}


