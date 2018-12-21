/*-----------------------  touch_home.c ------------------------------
1. Only first byte read from XPT is meaningful !!!??

2. Keep enough time-gap for XPT to prepare data in each read-session,
usleep 3000us seems OK, or the data will be too scattered.

3. Hold LCD-Touch module carefully, it may influence touch-read data.

4. TOUCH Y edge doesn't have good homogeneous property along X,
   try to draw a horizontal line and check visually, it will bend.

5. if sx0=sy0=0, it means that last TOUCH coordinate is invalid or pen-up.

6. Linear piont to piont mapping from TOUCH to LCD. There are
   unmapped points on LCD.

7. point to an oval area mapping form TOUCH to LCD.


TODO:
0. Cancel extern vars in all head file, add xx_get_xxVAR() functions.
1. Screen sleep
2. LCD_SIZE_X,Y to be cancelled. use FB parameter instead.
3. home page refresh button... 3s pressing test.
4. apply mutli-process for separative jobs: reading-touch-pad, CLOCK,texting,... etc.
5. systme()... sh -c ...
6. btn-press and btn-release signal
7. To read FBDE vinfo to get all screen/fb parameters as in fblines.c, it's improper in other source files.

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <signal.h>
#include "color.h"
#include "spi.h"
#include "fblines.h"
#include "egi.h"
#include "xpt2046.h"
#include "bmpjpg.h"
#include "dict.h"
#include "egi_timer.h"
#include "symbol.h"


/*  ---------------------  MAIN  ---------------------  */
int main(void)
{
	int i,j;
	int index;
	int ret;
	uint16_t sx,sy;  //current TOUCH point coordinate, it's a LCD screen coordinates derived from TOUCH coordinate.

	int delt=5; /* incremental value*/
	//char str_syscmd[100];
	//char strf[100];

	uint16_t *buf;
	buf=(uint16_t *)malloc(320*240*sizeof(uint16_t));

	uint16_t mag=COLOR_RGB_TO16BITS(255,0,255);
	printf("mag=%04x\n",mag);



	/* ------------ NOTE ebox test ------------------ */
	struct egi_data_txt note_txt={0};
	/* init txtbox data: offset(10,10) 2_lines, 510bytes per txt line,font, font_color */
	if( egi_init_data_txt(&note_txt, 5, 5, 2, 510, &sympg_testfont, WEGI_COLOR_BLACK) ==NULL ) {
		printf("init NOTE data txt fail!\n"); exit(1);
	 }
	struct egi_element_box ebox_note=
	{
		.type = type_txt,
		.egi_data =(void *) &note_txt,
		.height = 60, /* two line */
		.width = 230,
		.prmcolor = WEGI_COLOR_GRAY,/* <0, transparent */
		.x0= 5,
		.y0= 320-80,
		.tag="note pad",
	};

	/* ------------ CLOCK ebox test ------------------- */
	struct egi_data_txt clock_txt={0};
	/* init txtbox data: offset(x,y) 1_lines, 480bytes per txt line,font, font_color */
	if( egi_init_data_txt(&clock_txt, 0, 0, 1, 120, &sympg_numbfont,WEGI_COLOR_BROWN) ==NULL ) {
		printf("init CLOCK data txt fail!\n"); exit(1);
	 }
	struct egi_element_box ebox_clock=
	{
		.type = type_txt,
		.egi_data =(void *) &clock_txt,
		.height = 20, /* ebox height */
		.width = 120,
		.prmcolor = EGI_NOPRIM_COLOR, /*-1, if<0,transparent */
		.x0= 60,
		.y0= 320-38,
		.frame=-1, /* <0, no frame */
		.tag="timer txt",
	};

	/* ------------ MEMO ebox test ------------------ */
	struct egi_data_txt memo_txt={0};
	/* init txtbox data: txt offset(5,50) to box, 4_lines, 480bytes per txt line,font, font_color */
	if( egi_init_data_txt(&memo_txt, 5, 5, 4, 160, &sympg_testfont, WEGI_COLOR_BLACK) == NULL ) {
		printf("init MEMO data txt fail!\n"); exit(1);
	}
	struct egi_element_box ebox_memo=
	{
		.type = type_txt,
		.egi_data =(void *)&memo_txt, /* try &note_txt.....you may use other txt data  */
		.height = 30, /*box height, one line, will be ajusted according to numb of lines */
		.width = 160,
		.prmcolor = EGI_NOPRIM_COLOR,//WEGI_COLOR_ORANGE,
		.x0= 0,
		.y0= 25,
		.tag="memo stick",
	};


	/* ------------  BUTTON ebox  ------------------ */
	struct egi_element_box  ebox_buttons[9]={0};
	struct egi_data_btn home_btns[9]={0};
	for(i=0;i<3;i++) /* row of icon img */
	{
		for(j=0;j<3;j++) /* column of icon img */
		{
			home_btns[3*i+j].shape=square;
			home_btns[3*i+j].id=3*i+j;
			home_btns[3*i+j].offy=100+(15+60)*i;
			home_btns[3*i+j].offx=15+(15+60)*j;
			home_btns[3*i+j].icon=&sympg_icon;
			home_btns[3*i+j].icon_code=3*i+j;	/* symbol code number */
			/* hook to ebox model */
			ebox_buttons[3*i+j].type=type_button;
			ebox_buttons[3*i+j].egi_data=(void *)(home_btns+3*i+j);
			sprintf(ebox_buttons[3*i+j].tag,"button_%d",3*i+j);
		}
	}




	/* --- open spi dev --- */
	SPI_Open();

	/* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);

	/* --- clear screen with BLACK --- */
#if 0
	clear_screen(&gv_fb_dev,(0<<11|0<<5|0));
	fbset_color(0xffff);
	draw_filled_rect(&gv_fb_dev,0,0,20,20);
	exit(1);
#endif

	/* --- load screen paper --- */
	show_jpg("home.jpg",&gv_fb_dev,0,0,0); /*black on*/

	/* --- load symbol dict --- */
	//dict_display_img(&fb_dev,"dict.img");
	//if(dict_load_h20w15("/home/dict.img")==NULL)
	//{
	//	printf("Fail to load home page!\n");
	//	exit(-1);
	//}

	/* --- print and display symbols --- */
#if 0
	//dict_print_symb20x15(dict_h20w15);
	//for(i=0;i<10;i++)
	//	dict_writeFB_symb20x15(&gv_fb_dev,1,(30<<11|45<<5|10),i,30+i*15,320-40);
#endif


	/* --- load testfont ---- */
	if(symbol_load_page(&sympg_testfont)==NULL)
		exit(-2);
	/* --- load numbfont ---- */
	if(symbol_load_page(&sympg_numbfont)==NULL)
		exit(-2);
	/* --- load icons ---- */
	if(symbol_load_page(&sympg_icon)==NULL)
		exit(-2);



	/* --------- test:  print all symbols in the page --------*/
#if 0
	for(i=32;i<127;i++)
	{
		symbol_print_symbol(&sympg_testfont,i,0xffff);
		//getchar();
	}
#endif
#if 0
	for(i=48;i<58;i++)
		symbol_print_symbol(&sympg_numbfont,i,0x0);
#endif





#if 0 	/* ------------- generate  and draw ebox -------- */
	for(i=0;i<nrow;i++) /* row */
	{
		for(j=0;j<ncolumn;j++) /* column */
		{
			/* generate boxes */
			ebox[ncolumn*i+j].type=type_button;
			ebox[ncolumn*i+j].height=sbox;
			ebox[ncolumn*i+j].width=sbox;
			ebox[ncolumn*i+j].x0=startX+(j+1)*sgap+j*sbox;
			ebox[ncolumn*i+j].y0=startY+i*(sgap+sbox);
		}
	}

     /* ------------ draw the eboxes -------------- */
	for(i=0;i<nrow*ncolumn;i++)
	{
		/* color adjust for button */
		fbset_color( (30-i*5)<<11 | (50-i*8)<<5 | (i+1)*10 );/* R5-G6-B5 */
//		fbset_color( (35-i*5)<<11 | (55-i*5)<<5 | (i+1)*10 );/* R5-G6-B5 */
//		fbset_color( (15+i*5)<<11 | (55-i*5)<<5 | (i+1)*5 );/* R5-G6-B5 */
	    if(1)
	    {
                if(i==0)fbset_color((0<<11)|(60<<5)|30);
                if(i==1)fbset_color((30<<11)|(60<<5)|0);
                if(i==2)fbset_color((0<<11)|(0<<5)|30);
                if(i==3)fbset_color((30<<11)|(0<<5)|0);
                if(i==4)fbset_color((0<<11)|(60<<5)|0);
                if(i==5)fbset_color((30<<11)|(0<<5)|30);
	    }
		draw_filled_circle(&gv_fb_dev,ebox[i].x0+ebox[i].width/2,
			ebox[i].y0+ebox[i].height/2, ebox[i].height/2);

		fbset_color(0); //set black rim
		draw_circle(&gv_fb_dev,ebox[i].x0+ebox[i].width/2,
                        ebox[i].y0+ebox[i].height/2, ebox[i].height/2);
	}
#endif /* end of draw ebox */



#if 0 /* ----  test circle ----------*/
	fbset_color(WEGI_COLOR_OCEAN);
	draw_filled_circle(&gv_fb_dev,120,160,90);
	fbset_color(0);
	draw_circle(&gv_fb_dev,120,160,90);
exit(1);
#endif

	/* ----------- activate txt and note eboxes ---------*/
	/* note:
	   Be careful to activate eboxes in the correct sequence.!!!
	   activate static eboxes first(no bkimg), then mobile ones(with bkimg),
	*/
	/* ------ activate icons ----- */
	for(i=0;i<9;i++)
		egi_btnbox_activate(ebox_buttons+i);
	egi_txtbox_activate(&ebox_clock); /* no time string here...*/
	egi_txtbox_sleep(&ebox_clock);/* put to sleep */

	egi_txtbox_activate(&ebox_note);
        egi_txtbox_sleep(&ebox_note); /* put to sleep */
	egi_txtbox_activate(&ebox_note);/* wake up */
//exit(1);
	strncpy(memo_txt.txt[0],"MEMO:",12);
	strncpy(memo_txt.txt[1],"1. make Coffee.",20);
	strncpy(memo_txt.txt[2],"2. take a break.",20);
	strncpy(memo_txt.txt[3],"3. write codes",20);
	egi_txtbox_activate(&ebox_memo);


	/* ---- set timer for time display ---- */
	tm_settimer(500000);/* set timer interval interval */
	signal(SIGALRM, tm_sigroutine);
	tm_tick_settimer(TM_TICK_INTERVAL);
	signal(SIGALRM, tm_tick_sigroutine);


	/* ----- set default color ----- */
        fbset_color((30<<11)|(10<<5)|10);/* R5-G6-B5 */

	/*  test an array of circle */
#if 0
	for(i=0;i<6;i++)
	{
		if(i==0)fbset_color((0<<11)|(60<<5)|30);
		if(i==1)fbset_color((30<<11)|(60<<5)|0);
		if(i==2)fbset_color((0<<11)|(0<<5)|30);
		if(i==3)fbset_color((30<<11)|(0<<5)|0);
		if(i==4)fbset_color((0<<11)|(60<<5)|0);
		if(i==5)fbset_color((30<<11)|(0<<5)|30);

		draw_filled_circle(&gv_fb_dev,20+i*i*7,70,10+i*4);
	}
#endif

	/* --- copy partial fb mem to buf -----*/
	//fb_cpyto_buf(&gv_fb_dev, 100,0,150,320-1, buf);


/* ===============----------(((  MAIN LOOP  )))----------================= */
	while(1)
	{
		/*------ relate with number of touch-read samples -----*/
		usleep(3000); //3000

		/*--------- read XPT to get avg tft-LCD coordinate --------*/
		ret=xpt_getavg_xy(&sx,&sy); /* if fail to get touched tft-LCD xy */
		if(ret == XPT_READ_STATUS_GOING )
		{
			continue; /* continue to loop to finish reading touch data */
		}

		/* -------  put PEN-UP status events here !!!! ------- */
		else if(ret == XPT_READ_STATUS_PENUP )
		{
		  /*  Heavy load task MUST NOT put here ??? */
			/* get hour-min-sec and display */
			tm_get_strtime(tm_strbuf);

/* TODO: if NOTE and MEMO has the same interval value,then the later one will never be performed !!!
			/* refresh timer NOTE eboxe according to tick */
			if( tm_get_tickcount()%50 == 0 ) /* 30*TM_TICK_INTERVAL(10000us) */
			{
				printf("tick = %lld\n",tm_get_tickcount());
				if(ebox_note.x0 <=60  ) delt=10;
				if(ebox_note.x0 >=300 ) delt=-10;
				ebox_note.x0 += delt; //85 - (320-60)
				egi_txtbox_refresh(&ebox_note);
			}
			/* refresh MEMO eboxe according to tick */
			if( tm_get_tickcount()%20 == 0 ) /* 30*TM_TICK_INTERVAL(10000us) */
			{
				ebox_memo.x0 -= 10;
				egi_txtbox_refresh(&ebox_memo);
			}

			/* -----ONLY if tm changes, update txt and clock */
			if( strcmp(note_txt.txt[1],tm_strbuf) !=0 )
			{
				/* update NOTE ebox txt  */
				strncpy(note_txt.txt[1],tm_strbuf,10);
				/* -----refresh CLOCK ebox---- */
				//wirteFB_str20x15(&gv_fb_dev, 1, (30<<11|45<<5|10), tm_strbuf, 60, 320-38);
				strncpy(clock_txt.txt[0],tm_strbuf,10);
				clock_txt.color += (6<<8 | 4<<5 | 2 );
				egi_txtbox_refresh(&ebox_clock);
			}

			/* get year-mon-day and display */
			tm_get_strday(tm_strbuf);
			symbol_string_writeFB(&gv_fb_dev, &sympg_testfont,WEGI_COLOR_SPRINGGREEN,1,12,2,tm_strbuf);//32,90
			/* copy to note_txt */
			strncpy(note_txt.txt[0],tm_strbuf,22);

			continue; /* continue to loop to read touch data */
		}

		else if(ret == XPT_READ_STATUS_COMPLETE)
		{
			printf("--- XPT_READ_STATUS_COMPLETE ---\n");
			/* going on then to check and activate pressed button */

		}

	///////////////// -----------  Touch Event Handling  ----------- ///////////////
		/*---  get index of pressed ebox and activate the button ----*/
	    	//index=egi_get_boxindex(sx,sy,ebox,nrow*ncolumn);
	    	index=egi_get_boxindex(sx,sy,ebox_buttons,9);
		printf("get box index=%d\n",index);
		continue;

#if 0
		if(index>=0) /* if get meaningful index */
		{
			if(index==0)
			{
				printf("refresh fb now.\n");
				system("/tmp/tmp_app");
				exit(1);
			}
			else
			{
				printf("button[%d] pressed!\n",index);
				//sprintf(strf,"m_%d.jpg",index+1);
				//show_jpg(strf, &gv_fb_dev, 1, 0, 0);/*black off*/
				switch(index)
				{
					case 0:
						break;
					case 1:
						break;
					case 2:
						ebox_memo.y0 -= 10;
						break;
					case 3:
						ebox_memo.x0 -= 10;
						break;
					case 4:
						ebox_memo.x0 += 10;
						break;
					case 5:
						ebox_memo.y0 += 10;
						break;
				}
				/* refresh ebox */
				egi_txtbox_refresh(&ebox_memo);
			}
			//usleep(200000); //this will make touch points scattered.
		}/* end of if(index>=0) */
#endif

	} /* end of while() loop */

	/* release symbol mem page */
	symbol_release_page(&sympg_testfont);

	/* release dict mem */
	dict_release_h20w15();

	/* close fb dev */
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
        close(gv_fb_dev.fdfd);

	/* close spi dev */
	SPI_Close();
	return 0;
}
