#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>


#include <termios.h>
#include <unistd.h>
#include "kmods/FL2000/include/fl2000_ioctl.h"

#define	FL2K_NAME	"/dev/fl2000-0"
/*
 * definitions
 */
#define IMAGE_ASPECT_RATIO_16_10                0
#define IMAGE_ASPECT_RATIO_4_3                  1
#define IMAGE_ASPECT_RATIO_5_4                  2
#define IMAGE_ASPECT_RATIO_16_9                 3

#define MAX_FRAME_BUFFER_SIZE			1920*1080*3
#define	NUM_FRAME_BUFFERS			2

/*
 * data structures
 */
struct resolution {
	uint32_t	width;
	uint32_t	height;
};

/*
 * global variables
 */
struct	monitor_info monitor_info;
struct resolution  resolution_list[64];
uint32_t num_resolution;
uint8_t frame_buffers[NUM_FRAME_BUFFERS][MAX_FRAME_BUFFER_SIZE];
uint32_t mem_type = 0;

struct test_alloc phy_frame_buffers[NUM_FRAME_BUFFERS];

/*
 * implementation
 */
int kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
	if(ch != EOF) {
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}

/*--------------------------------------------
add valid resolution to resolution_list[]
--------------------------------------------*/
void add_resolution(uint32_t width, uint32_t height)
{
	unsigned int i;
	bool resolution_found;

	/*
	 * check if this resolution is already there
	 */
	resolution_found = false;
	for (i = 0; i < num_resolution; i++) {
		struct resolution * this = &resolution_list[i];

		if (this->width == width && this->height == height) {
			resolution_found = true;
			break;
		}
	}

	if (!resolution_found) {
		resolution_list[num_resolution].width = width;
		resolution_list[num_resolution].height = height;
		fprintf(stdout, "entry [%u] = (%u, %u) added\n",
			num_resolution, width, height);
		num_resolution++;
	}
}

/*-----------------------------------------------
decide Width Aand Height accroding to monitor EDID
ratio
------------------------------------------------*/
void fl2000_monitor_ratio_to_dimension(
	uint8_t x,
	uint8_t aspect_ratio,
	uint32_t* width,
	uint32_t* height)
{
	uint32_t temp_width;
	uint32_t temp_height;

	temp_width = (x + 31) * 8;
	switch (aspect_ratio) {
	case IMAGE_ASPECT_RATIO_16_10:
		printf("monitor ratio 16:10 -----\n");
		temp_height = (temp_width / 16) * 10;
		break;

	case IMAGE_ASPECT_RATIO_4_3:
		printf("monitor ratio 4:3 -----\n");
		temp_height = (temp_width / 4) * 3;
		break;

	case IMAGE_ASPECT_RATIO_5_4:
		printf("monitor ratio 16:10 -----\n");
	    temp_height = (temp_width / 5)*4; 
	    break;

	case IMAGE_ASPECT_RATIO_16_9:
	default:
		printf("monitor ratio 16:9 -----\n");
	    temp_height = ( temp_width / 16 ) * 9;
	    break;

	}

	*width = temp_width;
	*height = temp_height;
}



void parse_edid(void)
{
	uint8_t i;
	uint32_t width;
	uint32_t height;
	uint8_t * const monitor_edid = monitor_info.edid;

	/*
	 * EDID offset 38 ~ 53. Standard timing information. Upto 8 2-bytes.
	 * Unused fields are filled with 01 01
	 */
	for (i = 38; i < 53; i+= 2) {
		uint8_t	 x = monitor_edid[i];
		uint8_t  ratio = monitor_edid[i + 1] >> 6;
		uint8_t  freq = (monitor_edid[i + 1] & 0x3F) + 60;
                printf("EDID freq=%d\n",freq);
		//if (monitor_edid[i] == 1 && monitor_edid[i + 1] == 1)
		//	continue;//break;

		fl2000_monitor_ratio_to_dimension(
			x,
			ratio,
			&width,
			&height);

		fprintf(stdout, "found (%u, %u) @ %u fps\n",
			width, height, freq);

		add_resolution(width, height);
	}

	/*
	 * check detailed timing descriptor
	 */
	for (i = 54; i < 125; i+= 18) {
		uint8_t *  entry = &monitor_edid[i];
		uint32_t h_active, v_active;

		/*
		 * NOT detailed timing descriptor
		 */
		if (entry[0] == 0 && entry[1] == 0)
			break;

		h_active = (entry[4] >> 4) << 8 | entry[2];
		v_active = (entry[7] >> 4) << 8 | entry[5];

		fprintf(stdout, "found (%u, %u) detailed timing desc\n",
			h_active, v_active);
		add_resolution(h_active, v_active);
	}
}


void init_frame_by_test_pattern(
	uint8_t * frame_buffer,
	uint32_t width,
	uint32_t height)
{
	uint32_t  y;
	uint32_t  x;
	int color = 0;

	/*
	 * init frame buffer with 24 RGB format.
	 * draw color stripe with 10 lines of R, 10 lines of G, 10 lines of B,
	 * 10 lines of white...
	 */
	for (y = 0; y < height; y++) {
		uint32_t const pitch = width * 3;

		for (x = 0; x < width; x++) {
			uint8_t* pixel = frame_buffer + y * pitch + x * 3;

			switch (color) {
			case 0:
				pixel[0] = 0;
				pixel[1] = 0;
				pixel[2] = 0xFF;
				break;
			case 1:
				pixel[0] = 0;
				pixel[1] = 0xFF;
				pixel[2] = 0;
				break;
			case 2:
				pixel[0] = 0xFF;
				pixel[1] = 0;
				pixel[2] = 0;
				break;
			default:
				pixel[0] = 0xFF;
				pixel[1] = 0xFF;
				pixel[2] = 0xFF;
				break;
			}
		}
		if (y % 10 == 9)
			color = (color + 1) % 4;
	}
}

/*
 * return true if a test_%u_%u.bmp exist, and is 24bpp format.
*/
bool init_frame_by_bmp_file(
	uint8_t * frame_buffer,
	uint32_t width,
	uint32_t height,
	uint32_t index,
//+++++ add uint8_t color_format in bool init_frame_by_bmp_file()
	uint8_t color_format)
{
	uint32_t const frame_size = width * height * 3;
	char file_name[128];
	uint8_t header[54];
	size_t len;
	bool file_ok = false;
	FILE * bmp_file;
	uint32_t bmp_width;
	uint32_t bmp_height;
	uint32_t bmp_bpp;
	uint32_t i;
	uint32_t nread;
//++++++ add rgb_tmp iniinit_frame_by_bmp_file()
	uint8_t rgb_tmp[3];
	uint32_t j,k,m;
        uint8_t* rgb_line;

	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "test_%d_%d_%d.bmp",
		width, height, index);
	printf("-----------------open file: %s -------------------\n",file_name);
	bmp_file = fopen(file_name, "r");
	if (bmp_file == NULL) {
		//fprintf(stderr, "%s not exists\n", file_name);
		goto exit;
	}

	/*
	 * read the header
	 */
	len = fread(header, 1, 54, bmp_file);
	if (len != 54) {
		fprintf(stderr, "%s len(%d) != 54 ?\n",
			file_name, (int) len);
		goto exit;
	}

	/*
	 * check header
	 */
	if (header[0] != 'B' || header[1] != 'M' || header[10] != 0x36 ||
	    header[14] != 0x28) {
		fprintf(stderr, "invalid header\n");
		goto exit;
	}

	bmp_width  = header[20] << 16 | header[19] << 8 | header[18];
	bmp_height = header[24] << 16 | header[23] << 8 | header[22];
	printf("bmp_height=%d,  bmp_width=%d \n",bmp_height,bmp_width);
	if (bmp_width != width || bmp_height != height) {
		fprintf(stderr, "bmp width/height (%u,%u) mismatch!\n",
			bmp_width, bmp_height);
		goto exit;
	}

	bmp_bpp = header[28];
	if (bmp_bpp != 24) {
		fprintf(stderr, "bmp_bpp(%d) not 24?\n", bmp_bpp);
		goto exit;
	}

	/*
	 * read the frame buffer from offset 54, in reverse order.
	 */

         printf(" read the frame buffer from offset 54, in reverse order....\n");

 switch(color_format){
   case  COLOR_FORMAT_RGB_24: 
// RGB_24 : Each pixel is comprised of 3 bytes and B occurs on byte[0], and
//      G occurs on byte[1], and R occurs on byte[2]. This is the default format.
	for (i = 0; i < height; i++) {
		uint8_t * frame_offset;

		frame_offset = frame_buffer + (height - i - 1) * width * 3;
		nread=fread(frame_offset, 1, width * 3,  bmp_file);
		//printf("i=%d, nread=%d\n",i,nread);
	}
	break;

   case COLOR_FORMAT_RGB_16_565:
//-----  read 24BIT RGB data from bmp_file and covert to 16_565 FORMAT for frame_buffer
/* RGB_16_565: Each pixel is comprised by 16 bits in little endian order. The
*      byte[0] is the least significant byte. Color B occurs on bits[0~4] of
 *      byte[0], Color G occurs on bits[5~7] of byte[0] and bits[0~2] of byte[1],
 *      Color R occurs on bits[3~7] of byte[1]. This color format is not encourged
 *      since it gives worse user experience.
 */
	rgb_line=malloc(width*3);
	for (i = 0; i < height; i++) {
		nread=fread(rgb_line,1,width*3,bmp_file);
		//--for every line, covert to RBG_565 FORMAT  in reverse order......!!!!!
		for( j=(height-i-1)*width;j<(height-i)*width;j++){  //--- see j as  pixel No.
			m=j%width;
			//---RBG R in high position
 	                frame_buffer[2*j] = ( ( (rgb_line[3*m+1]<<3) & 0xE0 ) | rgb_line[3*m]>>3 );
	                frame_buffer[2*j+1] =( ( rgb_line[3*m+2] & 0xF8 ) | ( rgb_line[3*m+1]>>5 ) );
		}
         }
	free(rgb_line);
	break;

   case COLOR_FORMAT_RGB_16_555:
//-----  read 24BIT RGB data from bmp_file and covert to 16_555 FORMAT for frame_buffer
/* RGB_16_555: Each pixel is comprised by 16 bits in little endian order. The
 *      byte[0] is the least significant byte. Color B occurs on bits[0~4] of
 *      byte[0], Color G occurs on bits[5~7] of byte[0] and bits[0~1] of byte[1],
 *      Color R occurs on bits[2~6] of byte[1]. The most signicant bit of byte[1]
 *      is not used. This color format is not encourged since it delivers worse
 *      user experience.
*/
	rgb_line=malloc(width*3);
	for (i = 0; i < height; i++) {
		nread=fread(rgb_line,1,width*3,bmp_file);
		//--for every line, covert to RBG_565 FORMAT  in reverse order......!!!!!
		for( j=(height-i-1)*width;j<(height-i)*width;j++){  //--- see j as  pixel No.
			m=j%width;
 	                frame_buffer[2*j] = ( ( (rgb_line[3*m+1]<<2) & 0xE0 ) | rgb_line[3*m]>>3 );
	                frame_buffer[2*j+1] =( ( (rgb_line[3*m+2]>>1) & 0x7C ) | ( rgb_line[3*m+1]>>6 ) );
		}
         }
	free(rgb_line);
	break;

   default:
	printf("color format error!\n");
	goto exit;

}


	file_ok = true;

exit:
	if (bmp_file != NULL)
		fclose(bmp_file);
	return file_ok;
}

int _alloc_frame(int fd, struct test_alloc * phy_alloc, uint32_t size)
{
	int ret;

	memset(phy_alloc, 0, sizeof(*phy_alloc));
	phy_alloc->buffer_size = (uint64_t) size;
	ret = ioctl(fd, IOCTL_FL2000_TEST_ALLOC_SURFACE, phy_alloc);
	if (ret < 0)
		fprintf(stderr,
			"%s: IOCTL_FL2000_TEST_ALLOC_SURFACE failed %d\n",
			__func__, ret);

	return ret;
}

void _release_frame(int fd, struct test_alloc * phy_alloc)
{
	int ret;

	printf(" start _release_frame ....\n");
	ret = ioctl(fd, IOCTL_FL2000_TEST_RELEASE_SURFACE, phy_alloc);
	if (ret < 0)
		fprintf(stderr,
			"%s: IOCTL_FL2000_TEST_RELEASE_SURFACE failed %d\n",
			__func__, ret);
}

/*--------------------------------------------------------
  test with  bmp file with name width_height_index.bmp
----------------------------------------------------------*/
void test_display(int fd, uint32_t width, uint32_t height, uint32_t index)
{
	struct display_mode display_mode;
	struct surface_info surface_info;
	struct surface_update_info update_info;
	uint8_t * frame_buffer;
	int ret_val;
	int i;

	/*
	 * create surface
	 */
	memset(&surface_info, 0, sizeof(surface_info));
	switch (mem_type) {
	case SURFACE_TYPE_VIRTUAL_FRAGMENTED_VOLATILE:
	case SURFACE_TYPE_VIRTUAL_FRAGMENTED_PERSISTENT:
		frame_buffer = frame_buffers[0];
		surface_info.handle		= (unsigned long) frame_buffer;
		surface_info.user_buffer	= (unsigned long) frame_buffer;
		printf("+++++---- surface_info.user_buffer = 0x%lx ----------\n",(unsigned long) surface_info.user_buffer);
		break;

	case SURFACE_TYPE_VIRTUAL_CONTIGUOUS:
		ret_val = _alloc_frame(fd, &phy_frame_buffers[0],
//+++++ change to  width * height * 2 if RGB_16 
			width * height * 3); //3);
		if (ret_val < 0)
			goto exit;

		fprintf(stderr,
			"usr_addr(0x%lx), phy_addr(0x%lx) allocated\n",
			(unsigned long) phy_frame_buffers[0].usr_addr,
			(unsigned long) phy_frame_buffers[0].phy_addr);
		frame_buffer = (uint8_t*) (unsigned long) phy_frame_buffers[0].usr_addr;
		surface_info.handle	 = phy_frame_buffers[0].usr_addr;
		surface_info.user_buffer = phy_frame_buffers[0].usr_addr;
		printf("+++++---- surface_info.user_buffer = 0x%lx ----------\n",(unsigned long) surface_info.user_buffer);
		break;

	case SURFACE_TYPE_PHYSICAL_CONTIGUOUS:
		ret_val = _alloc_frame(fd, &phy_frame_buffers[0],
//+++++ change to  width * height * 2 if RGB_16 
			width * height * 3); //3);
		if (ret_val < 0)
			goto exit;

		fprintf(stderr,
			"usr_addr(0x%lx), phy_addr(0x%lx) allocated\n",
			(unsigned long) phy_frame_buffers[0].usr_addr,
			(unsigned long) phy_frame_buffers[0].phy_addr);
		frame_buffer = (uint8_t*) (unsigned long) phy_frame_buffers[0].usr_addr;
		surface_info.handle	 = phy_frame_buffers[0].usr_addr;
		surface_info.user_buffer = phy_frame_buffers[0].phy_addr;
		printf("+++++---- surface_info.handle  = phy_frame_buffers[0].usr_addr  = 0x%lx ----------\n",(unsigned long) surface_info.handle);
		printf("+++++---- surface_info.user_buffer = phy_frame_buffers[0].phy_addr  = 0x%lx ----------\n",(unsigned long) surface_info.user_buffer);
		break;
	}
//=======================   here may add a loop for read bmp files and notify surface update ======================
//+++++  Move following codes to  just after  IOCTL_FL2000_SET_DISPLAY_MODE.
//first you have to memset() to force real mem. allocation
	memset(frame_buffer, 0, width*height*2);
/*
	printf("start init_frame_by bmp_file OR test_pattern.....\n");
	if (!init_frame_by_bmp_file(frame_buffer, width, height, index))
	{
		printf("!init_frame_by_bmp_file(),try to init_frame_by_test_pattern()....\n");
		init_frame_by_test_pattern(frame_buffer, width, height);
	}
*/
///----------------------------------------------------------------------///

//------surface_info.buffer_length: *2 for RGB_16 and *3 for RGB_24
// !!!!!!!!!!! surface_info.buffer_length MUST ACCORD WITH  size of  _alloc_frame() above !!!!!
// or
	surface_info.buffer_length	= width * height * 2; //2 for RGB_16 and 3 for RGB_24
	surface_info.width		= width;
	surface_info.height		= height;
//---- change pitch and color_format
// !!!!  surface_info.pitch and color_format will be used to create surface and pin down pages
	surface_info.pitch		= width * 2;   //-----2 for RGB_16 and 3 for RGB_24,-------- width*(bytes per pixle)
	surface_info.color_format	= COLOR_FORMAT_RGB_16_555;//565;//COLOR_FORMAT_RGB_24; 
	surface_info.type		= mem_type;

	fprintf(stderr, "create_surface(%u, %u) , type(0x%x)\n",
		width, height, surface_info.type);

	printf("start create surface....\n");
	ret_val = ioctl(fd, IOCTL_FL2000_CREATE_SURFACE, &surface_info);
	if (ret_val < 0) {
		fprintf(stderr, "IOCTL_FL2000_CREATE_SURFACE failed %d\n",
			ret_val);
		goto exit;
	}

	/*
	 * set display mode
	 */
	memset(&display_mode, 0, sizeof(display_mode));
	display_mode.width = width;
	display_mode.height = height;
	display_mode.refresh_rate = 60;//60  666666666666
	// input_color_format will be used to get vr_params.input_bytes_per_pixel = 2;
	//and input_bytes_per_pixel will finally be used in fl2000_compression.c
	display_mode.input_color_format = COLOR_FORMAT_RGB_16_555;//565;//24;
	//+++++ this setting must comply with fl2000_dongle.c
	//output_color  COLOR_FORMAT_ will be used to search corresponding VGA_BIG_TABLE_ in fl2000_dongle_set_params() of fl2000_dongle.c
	//output_color_format will finally be used to fl2000_reg_bit_set() in fl2000_monitor.c
	display_mode.output_color_format =COLOR_FORMAT_RGB_16_555;//565;//24;//16_555;//16_565;//COLOR_FORMAT_RGB_24;

	ret_val = ioctl(fd, IOCTL_FL2000_SET_DISPLAY_MODE, &display_mode);
	if (ret_val < 0) {
		fprintf(stderr, "IOCTL_FL2000_SET_DISPLAY_MODE failed %d\n",
			ret_val);
		goto exit;
	}
//+++++----- ===========  move init_frame_by_bmp_file() call to just after  IOCTL_FL2000_SET_DISPLAY_MODE.....
	printf("start init_frame_by bmp_file OR test_pattern.....\n");
	if (!init_frame_by_bmp_file(frame_buffer, width, height, index,display_mode.input_color_format))
	{
		printf("!init_frame_by_bmp_file(),try to init_frame_by_test_pattern()....\n");
		init_frame_by_test_pattern(frame_buffer, width, height);
	}
//----------------------------------//

	/*
	 * send update to driver
	 */
	memset(&update_info, 0, sizeof(update_info));
	update_info.handle 		= surface_info.handle;
	update_info.user_buffer 	= surface_info.user_buffer;
//+++++  if NOT 24BIT format then update_info.buffer_length is not width * height*3bytes !!!!
	if(display_mode.output_color_format == COLOR_FORMAT_RGB_16_565 || display_mode.output_color_format == COLOR_FORMAT_RGB_16_555){
//+++++----- adjust URB bufferlen, update_info.buffer_length = width * height*1
		update_info.buffer_length       = width * height*2;// * 2;
	}
	else{
		update_info.buffer_length 	= width * height * 3;
	}
  //-----------------------------------------

       fprintf(stdout, "IOCTL_FL2000_NOTIFY_SURFACE_UPDATE: display_mode(%u, %u)\n",width,height);

//+++++----- start a for loop to update surface...
//for(i=0;i<2500;i++)
{
	ret_val = ioctl(fd, IOCTL_FL2000_NOTIFY_SURFACE_UPDATE, &update_info);
	if (ret_val < 0) {
		fprintf(stderr, "IOCTL_FL2000_NOTIFY_SURFACE_UPDATE failed %d\n",
			ret_val);
		goto exit;
	}
	printf("-----ioctl NOTIFY_SURFACE_UPDATE: i=%d  usleep()....------\n",i);
}
//////////////////////////////

	fprintf(stdout, "display_mode(%u, %u), press any key to continue\n",
		width, height);

	while (kbhit() == 0)
		usleep(1000*100);
	getchar();

	/*
	 * disable output
	 */
//+++++ Don't _SET_DISPLAY_MODE to 0 after render 

        printf("disable output-----\n");
	//--set all param 0, as for shut down rending --
	memset(&display_mode, 0, sizeof(display_mode));
	ret_val = ioctl(fd, IOCTL_FL2000_SET_DISPLAY_MODE, &display_mode);


	/*
	 * destroy surface
	 */

	printf("destroy surface -----\n");
	ret_val = ioctl(fd, IOCTL_FL2000_DESTROY_SURFACE, &surface_info);
	if (ret_val < 0) {
		fprintf(stderr, "IOCTL_FL2000_DESTROY_SURFACE failed %d\n",
			ret_val);
		goto exit;
	}


exit:
	switch (mem_type) {
	case SURFACE_TYPE_VIRTUAL_CONTIGUOUS:
	case SURFACE_TYPE_PHYSICAL_CONTIGUOUS:
		if (phy_frame_buffers[0].usr_addr != 0)
			_release_frame(fd, &phy_frame_buffers[0]);
		break;
	}
}

void test_display_all(int fd)
{
	int i;

	for (i = num_resolution - 1; i >= 0; i--) {
		struct resolution * this = &resolution_list[i];

		test_display(fd, this->width, this->height, 0);
	}
}

bool fl2000_is_connected(void)
{
	struct stat f_stat;
	int    ret_val;

	ret_val = stat(FL2K_NAME, &f_stat);

	if (ret_val == 0)
		return true;
	return false;
}



void test_display_on_resolution(int fd, uint32_t width, uint32_t height)
{
	struct display_mode display_mode;
	struct surface_info surface_info;
	struct surface_update_info update_info;
	uint8_t * frame_buffer;
	int ret_val;
	int index;
	bool bmp_ok;
	unsigned int num_bmp;


	 // find how many bmp files are available, look for at most
	 // 16 test_xxxx_yyyy_z.bmp
 
	num_bmp = 0;
	for (index = 0; index < NUM_FRAME_BUFFERS; index++) {
		char file_name[128];
		FILE * bmp_file;

		memset(file_name, 0, sizeof(file_name));
		snprintf(file_name, sizeof(file_name), "test_%d_%d_%d.bmp",
			width, height, index);
		bmp_file = fopen(file_name, "r");
		if (bmp_file == NULL)
			break;
		fclose(bmp_file);
		num_bmp++;
	}



	// set display mode

	memset(&display_mode, 0, sizeof(display_mode));
	display_mode.width = width;
	display_mode.height = height;
	display_mode.refresh_rate = 60;
	display_mode.input_color_format = COLOR_FORMAT_RGB_24;
	display_mode.output_color_format = COLOR_FORMAT_RGB_24;

	ret_val = ioctl(fd, IOCTL_FL2000_SET_DISPLAY_MODE, &display_mode);
	if (ret_val < 0) {
		fprintf(stderr, "IOCTL_FL2000_SET_DISPLAY_MODE failed %d\n",
			ret_val);
		goto exit;
	}


 	// create surfaces
	for (index = 0; index < num_bmp; index++) {
		memset(&surface_info, 0, sizeof(surface_info));

		switch (mem_type) {
		case SURFACE_TYPE_VIRTUAL_FRAGMENTED_VOLATILE:
		case SURFACE_TYPE_VIRTUAL_FRAGMENTED_PERSISTENT:
			frame_buffer = frame_buffers[index];
			init_frame_by_bmp_file(frame_buffer, width, height, index,display_mode.input_color_format);
			surface_info.handle		= (unsigned long) frame_buffer;
			surface_info.user_buffer	= (unsigned long) frame_buffer;
			break;

		case SURFACE_TYPE_VIRTUAL_CONTIGUOUS:
			ret_val = _alloc_frame(fd, &phy_frame_buffers[index],
				width * height * 3);
			if (ret_val < 0)
				goto exit;

			fprintf(stderr,
				"usr_addr(0x%lx), phy_addr(0x%lx) allocated\n",
				(unsigned long) phy_frame_buffers[index].usr_addr,
				(unsigned long) phy_frame_buffers[index].phy_addr);
			frame_buffer = (uint8_t*) (unsigned long) phy_frame_buffers[index].usr_addr;
			init_frame_by_bmp_file(frame_buffer, width, height, index,display_mode.input_color_format);
			surface_info.handle	 = phy_frame_buffers[index].usr_addr;
			surface_info.user_buffer = phy_frame_buffers[index].usr_addr;
			break;
		case SURFACE_TYPE_PHYSICAL_CONTIGUOUS:
			ret_val = _alloc_frame(fd, &phy_frame_buffers[index],
				width * height * 3);
			if (ret_val < 0)
				goto exit;

			fprintf(stderr,
				"usr_addr(0x%lx), phy_addr(0x%lx) allocated\n",
				(unsigned long) phy_frame_buffers[index].usr_addr,
				(unsigned long) phy_frame_buffers[index].phy_addr);
			frame_buffer = (uint8_t*) (unsigned long) phy_frame_buffers[index].usr_addr;
			init_frame_by_bmp_file(frame_buffer, width, height, index,display_mode.input_color_format);
			surface_info.handle	 = phy_frame_buffers[index].usr_addr;
			surface_info.user_buffer = phy_frame_buffers[index].phy_addr;
			break;
		}

		surface_info.buffer_length	= width * height * 3;
		surface_info.width		= width;
		surface_info.height		= height;
		surface_info.pitch		= width * 3;
		surface_info.color_format	= COLOR_FORMAT_RGB_24;
		surface_info.type		= mem_type;
		ret_val = ioctl(fd, IOCTL_FL2000_CREATE_SURFACE, &surface_info);
		if (ret_val < 0) {
			fprintf(stderr, "IOCTL_FL2000_CREATE_SURFACE failed %d\n",
				ret_val);
			goto exit;
		}
	}

	 // for each primary surfaces, send update to kernel driver.
	index = 0;
	do {
		int c;

		if (!fl2000_is_connected())
			break;

		frame_buffer = frame_buffers[index];
		memset(&update_info, 0, sizeof(update_info));
		switch (mem_type) {
		case SURFACE_TYPE_VIRTUAL_FRAGMENTED_VOLATILE:
		case SURFACE_TYPE_VIRTUAL_FRAGMENTED_PERSISTENT:
			update_info.handle 	= (unsigned long) frame_buffer;
			update_info.user_buffer = (unsigned long) frame_buffer;
			break;

		case SURFACE_TYPE_VIRTUAL_CONTIGUOUS:
			update_info.handle	= phy_frame_buffers[index].usr_addr;
			update_info.user_buffer	= phy_frame_buffers[index].usr_addr;
			break;

		case SURFACE_TYPE_PHYSICAL_CONTIGUOUS:
			update_info.handle	= phy_frame_buffers[index].usr_addr;
			update_info.user_buffer	= phy_frame_buffers[index].phy_addr;
			break;
		default:
			fprintf(stderr, "unkown mem_type(%u)?\n", mem_type);
			break;
		}
		update_info.buffer_length 	= width * height * 3;

		ret_val = ioctl(fd, IOCTL_FL2000_NOTIFY_SURFACE_UPDATE, &update_info);
		if (ret_val < 0) {
			fprintf(stderr, "IOCTL_FL2000_NOTIFY_SURFACE_UPDATE failed %d\n",
				ret_val);
			goto exit;
		}

		if (++index >= num_bmp)
			index = 0;

		if (kbhit() == 0) {
			usleep(1000*10);	// sleep for 10 ms
			continue;
		}

		c = getchar();
		if (c == 27)
			break;
	} while (true);

	 // disable output
	memset(&display_mode, 0, sizeof(display_mode));
	ret_val = ioctl(fd, IOCTL_FL2000_SET_DISPLAY_MODE, &display_mode);

	 // destroy all surfaces
	for (index = 0; index < num_bmp; index++) {
		frame_buffer = frame_buffers[index];
		memset(&surface_info, 0, sizeof(surface_info));
		switch (mem_type) {
		case SURFACE_TYPE_VIRTUAL_FRAGMENTED_VOLATILE:
		case SURFACE_TYPE_VIRTUAL_FRAGMENTED_PERSISTENT:
			surface_info.handle		= (unsigned long) frame_buffer;
			surface_info.user_buffer	= (unsigned long) frame_buffer;
			break;
		case SURFACE_TYPE_VIRTUAL_CONTIGUOUS:
			_release_frame(fd, &phy_frame_buffers[index]);
			surface_info.handle		= phy_frame_buffers[index].usr_addr;
			surface_info.user_buffer	= phy_frame_buffers[index].usr_addr;
			break;
		case SURFACE_TYPE_PHYSICAL_CONTIGUOUS:
			_release_frame(fd, &phy_frame_buffers[index]);
			surface_info.handle		= phy_frame_buffers[index].usr_addr;
			surface_info.user_buffer	= phy_frame_buffers[index].phy_addr;
			break;
		}
		surface_info.buffer_length	= width * height * 3;
		surface_info.width		= width;
		surface_info.height		= height;
		surface_info.pitch		= width * 3;
		surface_info.color_format	= COLOR_FORMAT_RGB_24;
		surface_info.type		= mem_type;
		ret_val = ioctl(fd, IOCTL_FL2000_DESTROY_SURFACE, &surface_info);
		if (ret_val < 0) {
			fprintf(stderr, "IOCTL_FL2000_DESTROY_SURFACE failed %d\n",
				ret_val);
			goto exit;
		}
	}

exit:;
}


void main(int argc, char* argv[])
{
	int ch;
	int fd;
	int ioctl_ret;

	if (geteuid() != 0) {
		fprintf(stderr, "need root privilege. try sudo %s\n", argv[0]);
		return;
	}

	fd = open(FL2K_NAME, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "fl2000 device not connected?\n");
		return;
	}

	ioctl_ret = ioctl(fd, IOCTL_FL2000_QUERY_MONITOR_INFO, &monitor_info);
	if (ioctl_ret < 0) {
		fprintf(stderr, "IOCTL_FL2000_QUERY_MONITOR_INFO fails %d\n",
			ioctl_ret);
		goto exit;
	}

	if (monitor_info.monitor_flags.connected == 0) {
		fprintf(stderr, "no monitor connected to FL2000?\n");
		goto exit;
	}

	if (argc < 2) {
usage:
		fprintf(stderr, "usage: %s mem_type [[width] [height]]\n"
			"mem_type is 0..3\n", argv[0]);
		fprintf(stderr,
			"eg1: to test with SURFACE_TYPE_VIRTUAL_FRAGMENTED_VOLATILE, type\n"
			"%s 0\n", argv[0]);
		fprintf(stderr,
			"eg2: to test without SURFACE_TYPE_VIRTUAL_FRAGMENTED_PERSISTENT, type\n"
			"%s 1\n", argv[0]);
		fprintf(stderr,
			"eg3: to test 1920x1080 with SURFACE_TYPE_VIRTUAL_FRAGMENTED_PERSISTENT, type\n"
			"%s 1 1920 1080\n", argv[0]);
		goto exit;
	}

	if (argc >= 2) {
		switch (argv[1][0]) {
		case '0':
			mem_type = SURFACE_TYPE_VIRTUAL_FRAGMENTED_VOLATILE;
			break;
		case '1':
			mem_type = SURFACE_TYPE_VIRTUAL_FRAGMENTED_PERSISTENT;
			break;
		case '2':
			mem_type = SURFACE_TYPE_VIRTUAL_CONTIGUOUS;
			break;
		case '3':
			mem_type = SURFACE_TYPE_PHYSICAL_CONTIGUOUS;
			break;
		}
	}

	if (argc > 2 && argc < 4)
		goto usage;

	parse_edid();

	if (argc >= 4) {
		int width, height;

		width = atoi(argv[2]);
		height = atoi(argv[3]);
		if (width * height * 3 > MAX_FRAME_BUFFER_SIZE) {
			fprintf(stderr, "image (%d, %d) too large)\n",
				width, height);
			goto exit;
		}
		test_display_on_resolution(fd, width, height);
	}
	else
        {
		//test_display_all(fd);
		printf("---test_display()-----\n");
		//test_display(fd,400,300,0);
		test_display(fd,640,480,2);
//		test_display(fd,640,350,0);
//		test_display(fd,720,400,0);
//		test_display(fd,800,600,2);
		//test_display(fd,1280,768,1);
	}

exit:
	close(fd);
}
