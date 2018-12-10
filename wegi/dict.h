/*--------------------------------------------------
Dictionary for symbols

Midas Zhou
---------------------------------------------------*/
#include <stdint.h>
#include <linux/fb.h>
#include "fblines.h"

#define DICT_IMG_WIDTH 240
#define DICT_IMG_HEIGHT 320

#define DICT_NUM_LIMIT 10//32 /* number of symbols in dict data */

/* symbols dict data  */
//extern uint16_t dict_h20w15[DICT_NUM_LIMIT][20*15]; /* 20*15*2 bytes each symbol */
extern uint16_t *dict_h20w15;

/* -----------------  function  ------------------ */
uint16_t *dict_init_h20w15(void);
void dict_display_img(FBDEV *fb_dev,char *path);
uint16_t *dict_load_h20w15(char *path);
void dict_print_symb20x15(uint16_t *dict);
void dict_display_symb20x15(int blackoff, FBDEV *fb_dev, const uint16_t *symbol, int x0, int y0);
void dict_release_h20w15(void);
