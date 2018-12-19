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

	/* Frame buffer device */
//        FBDEV     fb_dev;

	/* ---- buttons array param ------*/
	int nrow=2; /* number of buttons at Y directions */
	int ncolumn=3; /* number of buttons at X directions */
	struct egi_element_box ebox[nrow*ncolumn]; /* square boxes for buttons */


	struct egi_element_box ebox_note={0};
	struct egi_data_txt note_txt={0};

	struct egi_element_box ebox_clock={0};
	struct egi_data_txt clock_txt={0};

	struct egi_element_box ebox_memo={0};
	struct egi_data_txt memo_txt={0};


	int startX=0;  /* start X of eboxes array */
	int startY=120; /* start Y of eboxes array */
	int sbox=70; /* side length of the square box */
	int sgap=(LCD_SIZE_X - ncolumn*sbox)/(ncolumn+1); /* gaps between boxes(bottons) */

	//char str_syscmd[100];
	char strf[100];

	uint16_t *buf;
	buf=(uint16_t *)malloc(320*240*sizeof(uint16_t));

	uint16_t mag=COLOR_RGB_TO16BITS(255,0,255);
	printf("mag=%04x\n",mag);

	/* --- open spi dev --- */
	SPI_Open();

	/* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);


	/* --- clear screen with BLACK --- */
/* do NOT clear, to avoid flashing */
#if 0
	clear_screen(&gv_fb_dev,(0<<11|0<<5|0));
	fbset_color(0xffff);
	draw_filled_rect(&gv_fb_dev,0,0,20,20);
	printf("draw_-50,-50,-150,-150\n");

//	int x1=50,y1=150,x2=150,y2=200;
	int x1=50,y1=0,x2=51,y2=1;
	fb_cpyto_buf(&gv_fb_dev,x1,y1,x2,y2, buf);
	for(i=1;i<10000;i++){
		fb_cpyfrom_buf(&gv_fb_dev,x1,y1,x2,y2, buf);
		y1 -= 15; y2 -= 15;
		fb_cpyto_buf(&gv_fb_dev,x1,y1,x2,y2, buf);
		draw_filled_rect(&gv_fb_dev,x1,y1,x2,y2);
		usleep(800000);
	}
	exit(1);
#endif

	/* --- load screen paper --- */
	show_jpg("home.jpg",&gv_fb_dev,0,0,0); /*black on*/
	//show_jpg("m_1.jpg",&gv_fb_dev,1,0,0); /* black off */
	//show_bmp("p2.bmp",&gv_fb_dev,1); /* black off */

	/* --- load symbol dict --- */
	//dict_display_img(&fb_dev,"dict.img");
	if(dict_load_h20w15("/home/dict.img")==NULL)
	{
		printf("Fail to load home page!\n");
		exit(-1);
	}
	/* --- print and display symbols --- */
#if 0
	dict_print_symb20x15(dict_h20w15);
	for(i=0;i<10;i++)
		dict_writeFB_symb20x15(&gv_fb_dev,1,(30<<11|45<<5|10),i,30+i*15,320-40);
#endif


	/* --- load testfont ---- */
	if(symbol_load_page(&sympg_testfont)==NULL)
		exit(-2);
	/* --- load numbfont ---- */
	if(symbol_load_page(&sympg_numbfont)==NULL)
		exit(-2);


	/* print all symbols in the page */
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


	/* ----- generate ebox parameters ----- */
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

	/* print box position for debug */
	// for(i=0;i<nrow*ncolumn;i++)
	//	printf("ebox[%d]: x0=%d, y0=%d\n",i,ebox[i].x0,ebox[i].y0);

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
		/*  draw filled rectangle */
//		draw_filled_rect(&gv_fb_dev,ebox[i].x0,ebox[i].y0,
//			ebox[i].x0+ebox[i].width-1,ebox[i].y0+ebox[i].height-1);
		/* or, draw filled circle */
		draw_filled_circle(&gv_fb_dev,ebox[i].x0+ebox[i].width/2,
			ebox[i].y0+ebox[i].height/2, ebox[i].height/2);

		fbset_color(0); //set black rim
		draw_circle(&gv_fb_dev,ebox[i].x0+ebox[i].width/2,
                        ebox[i].y0+ebox[i].height/2, ebox[i].height/2);
//		draw_circle(&gv_fb_dev,ebox[i].x0+ebox[i].width/2,
//                      ebox[i].y0+ebox[i].height/2, ebox[i].height/2-1);
	}

#if 0 /* ----  test circle ----------*/
	fbset_color(WEGI_COLOR_OCEAN);
	draw_filled_circle(&gv_fb_dev,120,160,90);
	fbset_color(0);
	draw_circle(&gv_fb_dev,120,160,90);

exit(1);
#endif

	/* ------------ CLOCK ebox test ------------------ ----
		1.  egi_init_data_txt()
		2.  then assign ebox_clock
		3.  activate teh exbox_clock
	*/
	/* init txtbox data: offset(x,y) 1_lines, 480bytes per txt line,font, font_color */
	egi_init_data_txt(&clock_txt, 0, 0, 1, 120, &sympg_numbfont,WEGI_COLOR_BROWN);//(30<<11|45<<5|10) );
	ebox_clock.type = type_txt;
	ebox_clock.egi_data =(void *) &clock_txt;
	ebox_clock.height = 20; /* ebox height */
	ebox_clock.width = 120;
	ebox_clock.prmcolor = -1; //-1, if<0,transparent   ( (0xEC&0xf8)<<8 | (0xEC&0xfc)<<3 | 0xEC>>3); /* 24bit E8E8E8 * <0 no prime color*/
	ebox_clock.x0= 60;
	ebox_clock.y0= 320-38;

	/* ------------ NOTE ebox test ------------------ */
	/* init txtbox data: offset(10,10) 2_lines, 510bytes per txt line,font, font_color */
	egi_init_data_txt(&note_txt, 5, 5, 2, 510, &sympg_testfont, 0);
	ebox_note.type = type_txt;
	ebox_note.egi_data =(void *) &note_txt;
	ebox_note.height = 60; /* two line */
	ebox_note.width = 230;
	ebox_note.prmcolor = WEGI_COLOR_GRAY;// (0xEC&0xf8)<<8 | (0xEC&0xfc)<<3 | 0xEC>>3; //0xffff; //-1, if<0,transparent   ( (0xEC&0xf8)<<8 | (0xEC&0xfc)<<3 | 0xEC>>3); /* 24bit E8E8E8 * <0 no prime color*/
	ebox_note.x0= 5;
	ebox_note.y0= 320-80;

	/* ------------ MEMO ebox test ------------------ */
	/* init txtbox data: txt offset(5,50) to box, 2_lines, 480bytes per txt line,font, font_color */
	egi_init_data_txt(&memo_txt, 5, 5, 2, 200, &sympg_testfont, 0);
	ebox_memo.type = type_txt;
	ebox_memo.egi_data =(void *)&memo_txt; /* try &note_txt.....you may use other txt data  */
	ebox_memo.height = 30; /*box height, one line, will be ajusted according to numb of lines */
	ebox_memo.width = 120;
	ebox_memo.prmcolor = WEGI_COLOR_ORANGE;// (0xEC&0xf8)<<8 | (0xEC&0xfc)<<3 | 0xEC>>3; //0xffff; //-1, if<0,transparent   ( (0xEC&0xf8)<<8 | (0xEC&0xfc)<<3 | 0xEC>>3); /* 24bit E8E8E8 * <0 no prime color*/
	ebox_memo.x0= 0;
	ebox_memo.y0= 50;


	/* activate eboxes */
	egi_txtbox_activate(&ebox_clock);
	egi_txtbox_activate(&ebox_note);
	egi_txtbox_activate(&ebox_memo);
	strncpy(memo_txt.txt[0],"MEMO:",12);
	strncpy(memo_txt.txt[1],"Make Coffee!",12);



	/* refresh txt box */
	//ebox_clock.y0 += 20;
	//egi_txtbox_refresh(&ebox_clock);

	/* ---- set timer for time display ---- */
	tm_settimer(500000);/* set timer interval interval */
	signal(SIGALRM, tm_sigroutine);

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
#if 1
		  /*  Heavy load task MUST NOT put here ??? */
			/* get hour-min-sec and display */
			tm_get_strtime(tm_strbuf);

			/* -----ONLY if tm changes, put in txtbox and refresh displaying */
			if( strcmp(note_txt.txt[1],tm_strbuf) !=0 )
			{
				/* refresh NOTE ebox */
				strncpy(note_txt.txt[1],tm_strbuf,10);
				//if(ebox_note.y0 > 320-1)ebox_note.y0=0;
				//ebox_note.x0+=10;
				ebox_note.y0 -= 10;
				egi_txtbox_refresh(&ebox_note);

				/* refresh CLOCK ebox */
				//wirteFB_str20x15(&gv_fb_dev, 1, (30<<11|45<<5|10), tm_strbuf, 60, 320-38);
				strncpy(clock_txt.txt[0],tm_strbuf,10);
				clock_txt.color += (15<<8 | 10<<5 | 5 );
				egi_txtbox_refresh(&ebox_clock);

				/* refre MEMO ebox */
				ebox_memo.x0 +=15;
				egi_txtbox_refresh(&ebox_memo);

			}

			/* get year-mon-day and display */
			tm_get_strday(tm_strbuf);
			symbol_string_writeFB(&gv_fb_dev, &sympg_testfont,COLOR_RGB_TO16BITS(0x33,0x99,0x33),1,30,2,tm_strbuf);//32,90
			/* copy to note_txt */
			strncpy(note_txt.txt[0],tm_strbuf,22);

#endif
			continue; /* continue to loop to read touch data */
		}



		else if(ret == XPT_READ_STATUS_COMPLETE)
		{
			printf("--- XPT_READ_STATUS_COMPLETE ---\n");

			/* going on then to check and activate pressed button */
		}


		////////// -----------  Touch Event Handling  ----------- //////////

		/*---  get index of pressed ebox and activate the button ----*/
	    	index=egi_get_boxindex(sx,sy,ebox,nrow*ncolumn);

		printf("get box index=%d\n",index);
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
				sprintf(strf,"m_%d.jpg",index+1);
				show_jpg(strf, &gv_fb_dev, 1, 0, 0);/*black off*/

			}
			//usleep(200000); //this will make touch points scattered.
		}

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
