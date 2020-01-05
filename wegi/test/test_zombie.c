/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Example:
1. Call egi_gif_slurpFile() to load a GIF file to an EGI_GIF struct
2. Call egi_gif_displayFrame() to display the EGI_GIF.
3. Call egi_gif_runDisplayThread() to display EGI_GIF by a thread.

Midas Zhou
------------------------------------------------------------------*/

#include <stdio.h>
#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "egi_gif.h"

static void display_gifCharacter( EGI_GIF_CONTEXT *gif_ctxt);


int main(int argc, char **argv)
{

        /* <<<<<  EGI general init  >>>>>> */
        printf("tm_start_egitick()...\n");
        tm_start_egitick();                     /* start sys tick */

        printf("egi_init_log()...\n");
        if(egi_init_log("/mmc/log_gif") != 0) {        /* start logger */
                printf("Fail to init logger,quit.\n");
                return -1;
	}

#if 0
        printf("symbol_load_allpages()...\n");
        if(symbol_load_allpages() !=0 ) {       /* load sys fonts */
                printf("Fail to load sym pages,quit.\n");
                return -2;
        }
        if(FTsymbol_load_appfonts() !=0 ) {     /* load FT fonts LIBS */
                printf("Fail to load FT appfonts, quit.\n");
                return -2;
        }
#endif

        printf("init_fbdev()...\n");
        if( init_fbdev(&gv_fb_dev) )            /* init sys FB */
                return -1;

#if 0
        printf("start touchread thread...\n");
        egi_start_touchread();                  /* start touch read thread */
#endif

        /* <<<<------------------  End EGI Init  ----------------->>>> */

	int i;
    	int xres;
    	int yres;
	EGI_GIF *gif_flower[2];
	EGI_GIF *gif_peashooter[1];
	EGI_GIF *gif_zombie[2];

//	bool DirectFB_ON=false; /* For transparency_off GIF,to speed up,tear line possible. */
//	bool ImgTransp_ON=true; /* Suggest: TURE */
//	int User_DispMode=-1;   /* <0, disable */
//	int User_TransColor=-1; /* <0, disable, if>=0, auto set User_DispMode to 2. */
//	int User_BkgColor=-1;   /* <0, disable */
//	int xp,yp;

	/* Set FB mode as LANDSCAPE  */
        fb_position_rotate(&gv_fb_dev, 3); //0);
    	xres=gv_fb_dev.pos_xres;
    	yres=gv_fb_dev.pos_yres;

	/* Load game bkg image and display it */
	EGI_IMGBUF* imgbuf_garden=egi_imgbuf_readfile("/mmc/zombie/pz_scene.png");
	if(imgbuf_garden==NULL) {
		printf("Fail to load imgbuf_garden!\n");
		exit(1);
	}
        egi_imgbuf_windisplay( imgbuf_garden, &gv_fb_dev, -1,                   /* img, fb, subcolor */
                               0, 0, 0, 0,                                      /* xp,yp  xw,yw */
                               imgbuf_garden->width, imgbuf_garden->height);    /* winw, winh */
        fb_page_refresh(&gv_fb_dev);

        /* refresh working buffer */
        //clear_screen(&gv_fb_dev, WEGI_COLOR_GRAY);

        /* init FB back ground buffer page */
        memcpy(gv_fb_dev.map_buff+gv_fb_dev.screensize, gv_fb_dev.map_fb, gv_fb_dev.screensize);
        /*  init FB working buffer page */
        //fbclear_bkBuff(&gv_fb_dev, WEGI_COLOR_BLUE);
        memcpy(gv_fb_dev.map_bk, gv_fb_dev.map_fb, gv_fb_dev.screensize);

	/* read GIF data into an EGI_GIF */
	for(i=0; i<2; i++) {
		gif_flower[i]= egi_gif_slurpFile("/mmc/zombie/sunflower.gif", true); /* fpath, bool ImgTransp_ON */
		if(gif_flower[i]==NULL) {
			printf("Fail to read in gif file for sunflowers!\n");
			exit(-1);
		}
	}
	for(i=0; i<1; i++) {
		gif_peashooter[i]= egi_gif_slurpFile("/mmc/zombie/peashooter.gif", true); /* fpath, bool ImgTransp_ON */
		if(gif_peashooter[i]==NULL) {
			printf("Fail to read in gif file for peashooters!\n");
			exit(-1);
		}
	}
	for(i=0; i<2; i++) {
		gif_zombie[i]= egi_gif_slurpFile("/mmc/zombie/coneheader.gif", true); /* fpath, bool ImgTransp_ON */
		if(gif_zombie[i]==NULL) {
			printf("Fail to read in gif file for cone zombies!\n");
			exit(-1);
		}
	}

        printf("Finishing read GIF file into EGI_GIF.\n");


	/* ------------ Plants EGI_GIF contxt -----------*/
	EGI_GIF_CONTEXT ctxt_flower[2];
	EGI_GIF_CONTEXT ctxt_peashooter[1];

        ctxt_flower[0]=(EGI_GIF_CONTEXT) {
  			.fbdev=NULL, //&gv_fb_dev;
        		.egif=gif_flower[0],
        		.nloop=-1,
        		.DirectFB_ON=false,
        		.User_DisposalMode=-1,
        		.User_TransColor=-1,
        		.User_BkgColor=-1,
        		.xp=gif_flower[0]->SWidth>xres ? (gif_flower[0]->SWidth-xres)/2:0,
        		.yp=gif_flower[0]->SHeight>yres ? (gif_flower[0]->SHeight-yres)/2:0,
        		.xw=10,
        		.yw=10,
        		.winw=gif_flower[0]->SWidth>xres ? xres:gif_flower[0]->SWidth,
        		.winh=gif_flower[0]->SHeight>yres ? yres:gif_flower[0]->SHeight
	};

	/* assign to other flowers */
	ctxt_flower[1]=ctxt_flower[0];
        ctxt_flower[1].egif=gif_flower[1];
        ctxt_flower[1].yw=145;
	ctxt_flower[1].egif->ImageCount=egi_random_max(ctxt_flower[1].egif->ImageTotal);

	/* assign to peashooter */
        ctxt_peashooter[0]=(EGI_GIF_CONTEXT) {
  			.fbdev=NULL, //&gv_fb_dev;
        		.egif=gif_peashooter[0],
        		.nloop=-1,
        		.DirectFB_ON=false,
        		.User_DisposalMode=-1,
        		.User_TransColor=-1,
        		.User_BkgColor=-1,
			.xp=gif_peashooter[0]->SWidth>xres ? (gif_peashooter[0]->SWidth-xres)/2:0,
			.yp=gif_peashooter[0]->SHeight>yres ? (gif_peashooter[0]->SHeight-yres)/2:0,
        		.xw=40,
        		.yw=35,
			.winw=gif_peashooter[0]->SWidth>xres ? xres:gif_peashooter[0]->SWidth,
			.winh=gif_peashooter[0]->SHeight>yres ? yres:gif_peashooter[0]->SHeight
	};


	/* ------------ Zombie EGI_GIF contxt -----------*/
	EGI_GIF_CONTEXT ctxt_zombie[2];

        ctxt_zombie[0]=(EGI_GIF_CONTEXT) {
  			.fbdev=NULL, //&gv_fb_dev;
        		.egif=gif_zombie[0],
        		.nloop=-1,
        		.DirectFB_ON=false,
        		.User_DisposalMode=-1,
        		.User_TransColor=-1,
        		.User_BkgColor=-1,
        		.xp=gif_zombie[0]->SWidth>xres ? (gif_zombie[0]->SWidth-xres)/2:0,
        		.yp=gif_zombie[0]->SHeight>yres ? (gif_zombie[0]->SHeight-yres)/2:0,
        		.xw=200,//300,
        		.yw=80,//60,
        		.winw=gif_zombie[0]->SWidth>xres ? xres:gif_zombie[0]->SWidth,
        		.winh=gif_zombie[0]->SHeight>yres ? yres:gif_zombie[0]->SHeight
	};
	ctxt_zombie[1]=ctxt_zombie[0];
        ctxt_zombie[1].egif=gif_zombie[1];
        ctxt_zombie[1].xw=250;
        ctxt_zombie[1].yw=0;
	ctxt_zombie[1].egif->ImageCount=egi_random_max(ctxt_zombie[1].egif->ImageTotal);

	/* --- start frame refresh thread for zombies --- */
	for(i=0; i<2; i++) {
		if( egi_gif_runDisplayThread(&ctxt_zombie[i]) !=0 ) {
			printf("Fail to run thread for ctxt_flower[%d]!\n",i);
			exit(1);
		} else {
			printf("Finish loading thread for ctxt_flower[%d]!\n",i);
		}
		tm_delayms(150); /* for some random factors */
	}


	/* --- start frame refresh thread for plants --- */
	for(i=0; i<2; i++) {
		if( egi_gif_runDisplayThread(&ctxt_flower[i]) !=0 ) {
			printf("Fail to run thread for ctxt_flower[%d]!\n",i);
			exit(1);
		} else {
			printf("Finish loading thread for ctxt_flower[%d]!\n",i);
		}
		tm_delayms(150); /* for some random factors */
	}
	for(i=0; i<1; i++) {
		if( egi_gif_runDisplayThread(&ctxt_peashooter[i]) !=0 ) {
			printf("Fail to run thread for ctxt_peashooter[%d]!\n",i);
			exit(1);
		} else {
			printf("Finish loading thread for ctxt_peashooter[%d]!\n",i);
		}
		tm_delayms(150); /* for some random factors */
	}



	/* ============================    GAME LOOP   =============================== */
while(1) {

        /* refresh working buffer */
        //fbclear_bkBuff(&gv_fb_dev, WEGI_COLOR_GRAY);
	memcpy(gv_fb_dev.map_bk, gv_fb_dev.map_buff+gv_fb_dev.screensize, gv_fb_dev.screensize);

	/* Beware of displaying sequence !*/

	/* ------------ Display Plants -------------- */
	/* display sunflowers */
	for(i=0; i<2; i++) {
		display_gifCharacter(&ctxt_flower[i]);
	}
	/* display peashooter */
	for(i=0; i<1; i++) {
		display_gifCharacter(&ctxt_peashooter[i]);
	}

	/* ------------ Display Zombies -------------- */
	/* display cone zombies */
	for(i=0; i<2; i++) {
		display_gifCharacter(&ctxt_zombie[i]);

		/* refresh position */
		ctxt_zombie[i].xw -= 1;
		if(ctxt_zombie[i].xw < -120 )
			ctxt_zombie[i].xw=300;
	}

	/* Refresh FB */
        fb_page_refresh(&gv_fb_dev);
	tm_delayms(100);
        //usleep(100000);
}


#if 0  /////////////////////////// TEST: egi_gif_runDisplayThread( )  ////////////////////////

while(1) {

	printf("\n\n\n\n----- New Round DisplayThread -----\n\n\n\n");

	/* start display thread */
	if( egi_gif_runDisplayThread(&gif_ctxt) !=0 )
		continue;

	/* Cancel thread after a while */
	//sleep(3);
	EGI_PLOG(LOGLV_CRITICAL,"%s: Start tm_delayms(3*1000) ... ",__func__);
	tm_delayms(3*1000);
	EGI_PLOG(LOGLV_CRITICAL,"%s: Finish tm_delayms(3*1000).", __func__);

	 printf("%s: Call egi_gif_stopDisplayThread...\n",__func__);
         if(egi_gif_stopDisplayThread(egif)!=0)
		EGI_PLOG(LOGLV_CRITICAL,"%s: Fail to call egi_gif_stopDisplayThread().",__func__);

 }
	exit(1);
#endif /////////////////////////////////////////////////////////////////////////////////////////



#if 0
        gif_ctxt.nloop=0; /* set one frame by one frame */

	/* Loop displaying */
        while(1) {

	    /* Display one frame/block each time, then refresh FB page.  */
	    egi_gif_displayFrame( &gif_ctxt );

        	gif_ctxt.xw -=2;

	}
    	egi_gif_free(&egif);
#endif 

	/* free EGI_GIF */
	egi_gif_free(&gif_flower[0]);
	egi_gif_free(&gif_flower[1]);

        /* <<<<<-----------------  EGI general release  ----------------->>>>> */
        printf("FTsymbol_release_allfonts()...\n");
        FTsymbol_release_allfonts();
        printf("symbol_release_allpages()...\n");
        symbol_release_allpages();
        printf("release_fbdev()...\n");
        fb_filo_flush(&gv_fb_dev); /* Flush FB filo if necessary */
        release_fbdev(&gv_fb_dev);
        printf("egi_end_touchread()...\n");
        egi_end_touchread();
        printf("egi_quit_log()...\n");

#if 0
        egi_quit_log();
        printf("<-------  END  ------>\n");
#endif

	return 0;
}

/* -----------------------------------------------
     Display GIF actor with its context
-------------------------------------------------*/
static void display_gifCharacter( EGI_GIF_CONTEXT *gif_ctxt)
{
        egi_imgbuf_windisplay( gif_ctxt->egif->Simgbuf, &gv_fb_dev, -1,    /* img, fb, subcolor */
                               gif_ctxt->xp, gif_ctxt->yp,            /* xp, yp */
                               gif_ctxt->xw, gif_ctxt->yw,            /* xw, yw */
                               gif_ctxt->winw, gif_ctxt->winh /* winw, winh */
                              );
}
