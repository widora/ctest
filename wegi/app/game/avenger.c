#include "egi_common.h"

typedef struct avg_plane_data AVG_PLANE;
struct avg_plane_data {
  const	EGI_IMGBUF	*icons;	 		/* Icons collection */
	int		icon_index;	 	/* Index of the icons for the plane image */
	EGI_POINT	pxy;	 		/* Current position*/
	int		speed;   		/* in pixles per refreshing */
	int (*trail_mode)(AVG_PLANE *)  	/* Method to refresh trail */
};

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
		plane->pxy.x=egi_random_max(240-50);
		plane->pxy.y=320;
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


	/* (const EGI_IMGBUF *egi_imgbuf, FBDEV *fb_dev, int subindex, int subcolor, int x0,   int y0) */
	egi_subimg_writeFB(plane->icons, &gv_fb_dev, plane->icon_index, -1, plane->pxy.x, plane->pxy.y);


	return 0;
}


void *thread_game_avenger(EGI_PAGE *page)
{

	int i,j;
	const char *fpath="/mmc/avenger/planes.png";
	EGI_IMGBOX *imboxes=NULL;
	EGI_IMGBUF *plane_icons=NULL;

	AVG_PLANE  planes[8];


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

	/* Prepare data for planes */
	for(i=0; i<8; i++) {
		planes[i].icons=plane_icons;
		planes[i].icon_index=i;	/* 0 for whole image, subimg from 1 */
		planes[i].pxy.x=egi_random_max(240-50);
		planes[i].pxy.y=320;
		planes[i].speed=egi_random_max(8)+2;
		planes[i].trail_mode=upward_trail;
	}


	/* 	----- GAME Loop -----	  */
	while(1) {

	        /* <<<<< Flush FB and Turn on FILO  >>>>> */
        	fb_filo_flush(&gv_fb_dev); /* flush and restore old FB pixel data */
	        fb_filo_on(&gv_fb_dev);    /* start collecting old FB pixel data */


		for(i=0; i<8; i++)
			refresh_plane(&planes[i]);


	        /* <<<<< Turn off FILO  >>>>> */
        	fb_filo_off(&gv_fb_dev);  /* Stop filo */
		tm_delayms(75);
	}


	/* Destroy facility and free resources */
	egi_imgbuf_free(plane_icons);

	return (void *)0;
}
