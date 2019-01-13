#include <stdlib.h>
#include "egi.h"
#include "egi_list.h"
#include "egi_timer.h"
#include "egi_objlist.h"
#include "egi_symbol.h"
#include "egi_color.h"


/*----------------------------------------------------------------
int reaction(EGI_EBOX *ebox, enum egi_touch_status status)
----------------------------------------------------------------*/
int egi_listbox_test(EGI_EBOX *ebox, enum egi_touch_status status)
{
	int i,j;
	int inum=5;
	int nl=2;
	static int count=0; /* test counter */

	printf("egi_listbox_test(): egi_list_new()... \n");
	/* 1. create a list ebox */
	EGI_EBOX *list=egi_listbox_new (
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
	EGI_DATA_LIST *data_list=(EGI_DATA_LIST *)(list->egi_data);
printf("egi_listbox_test(): finish egi_listbox_new(). \n");

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

	/* test counter */
	sprintf(&data[0][1][0],"count: %d", count++);

	char **pdata;
	pdata=malloc(nl*sizeof(char *)); //nl=2;

	// 0xCFF9,0xFE73,0x07FF,0xFE73,0xFE7F
//	uint16_t color[]= {0xDEFB, 0xFE73, 0x07FF, 0xFE73, 0xFE7F};
	uint16_t color[]= {0x67F9, 0xFFE6, 0x0679, 0xFE79, 0x9E6C};

	for(i=0; i<inum; i++)
	{
		/* set data */
		for(j=0;j<nl;j++) //nl=2
		{
			pdata[j]=&data[i][j][0];
			printf("pdata[%d]: %s \n",j,pdata[j]);
		}

		/* set icon */
		data_list->icons[i]=&sympg_icons;
		data_list->icon_code[i]=31;

		/* update items */
		egi_listbox_updateitem(list, i, color[i], pdata);
	}

	/* 4. activate the list	*/
	egi_listbox_activate(list);
	printf("egi_listbox_test(): finish egi_list_activate(). \n");

	/* 5. loop refresh */
	i=0;
	pdata[0]="Hello! NEO world!";

	while(1)
	{
		/* update item 0  */
		sprintf(pdata[1],"count: %d", i++);
		egi_listbox_updateitem(list, 0, -1, pdata); /* -1, keep old color */
		/* update item 3 */
		sprintf(pdata[1],"count: %d", i++);
		egi_listbox_updateitem(list, 3, -1, pdata); /* -1, keep old color */

		egi_listbox_refresh(list);
		tm_delayms(55);
	}


	printf("egi_listbox_test(): start list->free(list)... \n");
	list->free(list);
	printf("egi_listbox_test(): start free(pdata)... \n");
	free(pdata);

	return 0;
}


