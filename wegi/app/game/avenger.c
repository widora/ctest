#include "egi_common.h"

typedef struct avg_plane_data AVG_PLANE;
struct avg_plane_data {
  const	EGI_IMGBUF	*icons;	 		/* Icons collection */
	int		icon_index;	 	/* Index of the icons for the plane image */
	EGI_IMGBUF	*refimg;		/* Loaded image as for refrence */
	EGI_IMGBUF	*actimg;		/* Image for displaying */
	EGI_POINT	pxy;	 		/* Current position, image center hopefully.*/
	int		heading;		/* Current heading, in Degree */
	int		speed;   		/* Current speed, in pixles per refreshing */
	int (*trail_mode)(AVG_PLANE *);w  	/* Method to refresh trail */
};


/* --------------------------------------------------------------
		Create a new plane
Return:
	Pointer to an AVG_PLANE		OK
	NULL				Fails
-----------------------------------------------------------------*/
AVG_PLANE* avg_create_plane(    EGI_IMGBUF *icons,  int icon_index,
				EGI_POINT pxy, int heading, int speed,
				int (*trail_mode)(AVG_PLANE *)
			    )
{
	AVG_PLANE   *plane=NULL;
	EGI_IMGBUF  *refimg=NULL;
	EGI_IMGBUF  *actimg=NULL;

	/* Check input data */
	if( icons==NULL || icons->subimgs==NULL || icon_index<0 || icon_index > icons->submax ) {
		printf("%s:Input icons is NULL or icon_index invalid!\n",__func__);
		return NULL;
	}

	/* Copy icons->subimg[] to refimg */
	refimg=egi_imgbuf_subImgCopy( icons, icon_index );
	if(refimg==NULL)
		return NULL;

	/* Create actimg according to heading */
	actimg=egi_imgbuf_rotate(refimg, heading);
	if(actimg==NULL) {
		egi_imgbuf_free(refimg);
		return NULL;
	}

	/* Calloc AVG_PLANE */
	plane=calloc(1, sizeof(AVG_PLANE));
	if(plane==NULL) {
		printf("%s:Fail to call calloc() !\n",__func__);
		egi_imgbuf_free(refimg);
		egi_imgbuf_free(actimg);
		return NULL;
	}

	/* Assign memebers */
	plane->icons=icons;
	plane->icon_index=icon_index;
	plane->refimg=refimg;
	plane->actimg=actimg;
	plane->pxy=pxy;
	plane->heading=heading;
	plane->speed=speed;
	plane->trail_mode=trail_mode;

	return plane;
}


/*--------------------------------------
	Destroy a plane
---------------------------------------*/
void avg_destroy_plane(AVG_PLANE **plane)
{
	if(*plane==NULL)
		return;

	if( (*plane)->refimg != NULL) {
		free((*plane)->refimg);
		(*plane)->refimg=NULL;
	}

	if( (*plane)->actimg != NULL) {
		free((*plane)->actimg);
		(*plane)->actimg=NULL;
	}

	free(*plane);
	*plane=NULL;
}


/* ------------------------------
A straight upward trail.
--------------------------------*/
static int upward_trail(AVG_PLANE *plane)
{
	if(plane==NULL)
		return -1;

	plane->pxy.y -= plane->speed;

	/* loop */
	if(plane->pxy.y<-50) {
		plane->pxy.x=egi_random_max(240)-25;
		plane->pxy.y=320;
		plane->speed=egi_random_max(12)+2;
	}

	return 0;
}


/* ------------------------------
Refresh a plane image on screen.
--------------------------------*/
inline static int refresh_plane(AVG_PLANE *plane)
{
	if(plane==NULL || plane->icons==NULL)
		return -1;

	/* update trail */
	if(plane->trail_mode != NULL)
		plane->trail_mode(plane);

	/* Refresh image */
        egi_imgbuf_windisplay( 	plane->actimg, &gv_fb_dev, -1,           /* img, fb, subcolor */
                               	0, 0,					 /* xp,yp */
				plane->pxy.x + plane->actimg->width/2,		 /* xw */
				plane->pxy.y + plane->actimg->height/2,          /* yw */
                               	plane->actimg->width, plane->actimg->height );   /* winw, winh */

	/* (const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subindex, int subcolor, int x0,   int y0) */
	//egi_subimg_writeFB(plane->icons, &gv_fb_dev, plane->icon_index, -1, plane->pxy.x, plane->pxy.y);

	return 0;
}


void *thread_game_avenger(EGI_PAGE *page)
{

	int i,j;
	const char *fpath="/mmc/avenger/planes.png";
	EGI_IMGBOX *imboxes=NULL;
	EGI_IMGBUF *plane_icons=NULL;

	AVG_PLANE  *planes[15];

	printf("Start GAME avenger...\n");

	/* Subimage definition: 4 colums, 2 rows of subimages */
	imboxes=egi_imgboxes_alloc(1+8);
	if(imboxes==NULL) {
		printf("%s: Fail to alloc imgboxes!\n",__func__);
		return (void *)-1;
	}

	for(i=0; i<2; i++) {
		for(j=0; j<4; j++) {	/* Note: imboxes[0] is for whole image, meaningless though. */
			imboxes[i*4+j]=(EGI_IMGBOX){ j*50, i*40, 50, 40 };
		}
	}

	/* read in icons file */
	printf("read in icons...\n");
        plane_icons=egi_imgbuf_readfile(fpath);
        if(plane_icons==NULL) {
                printf("%s: Fail to read image file '%s'.\n", __func__, fpath);
		egi_imgboxes_free(imboxes);
		return (void *)-2;
        }

	/* Set subimage */
	plane_icons->subimgs=imboxes; imboxes=NULL; /* Ownership transferred */
	plane_icons->submax=8-1;


#if 0	/* Prepare data for planes */
	for(i=0; i<15; i++) {
		planes[i].icons=plane_icons;
		planes[i].icon_index=egi_random_max(8)-1;
		planes[i].pxy.x=egi_random_max(240)-25;
		planes[i].pxy.y=320;
		planes[i].speed=egi_random_max(12)+1;
		planes[i].trail_mode=upward_trail;
	}
#endif

	/* Create planes */
	for(i=0; i<15; i++)  {
		planes[i]=avg_create_plane( plane_icons, egi_random_max(8)-1,   /* EGI_IMGBUF *icons, icon_index */
       	                              (EGI_POINT){egi_random_max(240)-25, 320},	/* EGI_POINT pxy */
		    			    egi_random_max(360),		/* int heading */
					    egi_random_max(12)+1,		/* int speed */
					    upward_trail		/* int (*trail_mode)(AVG_PLANE *) */
                        	    	 );

	}

	/* 	----- GAME Loop -----	  */
	while(1) {

	        /* <<<<< Flush FB and Turn on FILO  >>>>> */
        	fb_filo_flush(&gv_fb_dev); /* flush and restore old FB pixel data */
	        fb_filo_on(&gv_fb_dev);    /* start collecting old FB pixel data */


		for(i=0; i<15; i++)
			refresh_plane(planes[i]);


	        /* <<<<< Turn off FILO  >>>>> */
        	fb_filo_off(&gv_fb_dev);  /* Stop filo */
		tm_delayms(75);
	}



	/* Destroy facility and free resources */
	egi_imgbuf_free(plane_icons);
	for(i=0; i<15; i++)
		avg_destroy_plane(&planes[i]);

	return (void *)0;
}
