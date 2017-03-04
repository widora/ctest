/*-------------------------------------------------
 RM68140 controlled LCD display  and  pin-control functions

------------------------------------------------*/
#ifndef _RM68140_H
#define _RM68140_H

#include "kspi.h"
//#define SPIBUFF_SIZE 36  //defined in kspi_draw.c


/* ---------  DCX pin:  0 as  command, 1 as data --------- */
 #define DCXdata base_gpio_set(14,1)
 #define DCXcmd base_gpio_set(14,0)

/* --------- Hardware set and reset pin-------------- */
 #define HD_SET base_gpio_set(15,1) //--pin 16 conflict with lirc module !!!!!!!
 #define HD_RESET base_gpio_set(15,0)


static void delayms(int s)
{
 int k;
 for(k=0;k<s;k++)
 {
    usleep(1000);
 }
}


/* -----  hardware reset ----- */
void LCD_HD_reset(void)
{
        HD_RESET;
        delayms(30);
        HD_SET;
        delayms(10);
}


/*---------- SPI_Write() -----------------*/
inline void SPI_Write(uint8_t *data,uint8_t len)
{
	int k,tx_len;
	if(len > 36) //MAX 32x9 (36bytes)data registers in SPI controller
		tx_len=36;
	else
		tx_len=len;

	for(k=0;k<tx_len;k++)
	{
 		spi_LCD.tx_buf[k]=*(data+k); //spi_LCD defined in kdraw.h and init in kspi_draw.c
	}
	spi_LCD.tx_len=tx_len;
	spi_LCD.rx_len=0;
	base_spi_transfer_half_duplex(&spi_LCD);
}

/*------ write command to LCD -------------*/
static void WriteComm(uint8_t cmd)
{
 DCXcmd;
 SPI_Write(&cmd,1);
}

/*------ write 1 Byte data to LCD -------------*/
static void WriteData(uint8_t data)
{
 DCXdata;
 SPI_Write(&data,1);
}
/*-------- write 2 Byte date to LCD ---------*/
static void WriteDData(uint16_t data)
{
  uint8_t tmp[2];
  tmp[0]=data>>8;
  tmp[1]=data&0x00ff;
 
  DCXdata;
  SPI_Write(tmp,2);
}

/*------ write N Bytes data to LCD -------------*/
static void WriteNData(uint8_t* data,int N)
{
 DCXdata;
 SPI_Write(data,N);
}


/*--------------- prepare for RAM direct write --------*/
static void LCD_ramWR_Start(void)
{
  WriteComm(0x2c);  
} 



/* ------------------ LCD Initialize Function ---------------*/
static void LCD_INIT_RM68140(void)
{
 delayms(20);
 LCD_HD_reset();

 WriteComm(0x11); // sleep out
 delayms(10); // must wait 5ms for sleep out

 WriteComm(0xc0);
 WriteComm(0xB7); 	//t ver address	
 WriteData(0x07);


 WriteComm(0xF0); 	//Enter ENG	
 WriteData(0x36);
 WriteData(0xA5);
 WriteData(0xD3);


 WriteComm(0xE5); 	//Open gamma function	
 WriteData(0x80);

 WriteComm(0xF0); 	//	Exit ENG	
 WriteData(0x36);
 WriteData(0xA5);
 WriteData(0x53);

 WriteComm(0xE0); 	//Gamma setting	
 WriteData(0x00);
 WriteData(0x35);
 WriteData(0x33);
 WriteData(0x00);
 WriteData(0x00);
 WriteData(0x00);
 WriteData(0x00);
 WriteData(0x35);
 WriteData(0x33);
 WriteData(0x00);
 WriteData(0x00);
 WriteData(0x00);

 WriteComm(0x3a); 	//interface pixel format	
 WriteData(0x55);      //16bits pixel 

/* --------- brightness control ----------*/

 WriteComm(0x55);WriteData(0x02); //-- adaptive brightness control Still Mode
 WriteComm(0x53);WriteData(0x2c); //--- Display control 
 WriteComm(0x5E);WriteData(0x00); //--- Min Brightness 
 WriteComm(0x51); WriteData(0x00); //-- display brightness
 printk("\nBrightness set finished!\n");


 WriteComm(0x29); 	//display ON
 delayms(10);

 WriteComm(0x36); //memory data access control
 WriteData(0x08); // BGR order ????? 
  //how to turn on off  back light

}

/* ------------- GRAM block address set ---------------- */
static void GRAM_Block_Set(uint16_t Xstart,uint16_t Xend,uint16_t Ystart,uint16_t Yend)
{
	WriteComm(0x2a);   
	WriteData(Xstart>>8);
	WriteData(Xstart&0xff);
	WriteData(Xend>>8);
	WriteData(Xend&0xff);

	WriteComm(0x2b);   
	WriteData(Ystart>>8);
	WriteData(Ystart&0xff);
	WriteData(Yend>>8);
	WriteData(Yend&0xff);
}


/* ------------  draw a color box --------------*/
static void LCD_ColorBox(uint16_t xStart,uint16_t yStart,uint16_t xLong,uint16_t yLong,uint16_t Color)
{
	uint32_t temp;
	u8 data[36];
	u8 hcolor,lcolor;
	int k;

	hcolor=Color>>8;
	lcolor=Color&0x00ff; 

        for(k=0;k<18;k++)
	{
		data[2*k]=hcolor;
		data[2*k+1]=lcolor;
	}

	GRAM_Block_Set(xStart,xStart+xLong-1,yStart,yStart+yLong-1);
        WriteComm(0x2c);  // ----for continous GRAM write

	temp=(xLong*yLong*2/36);
        for(k=0;k<temp;k++)
		WriteNData(data,36);//36*u8 = 18*u16
	temp=(xLong*yLong)%18;
	WriteNData(data,temp*2);

/*
	GRAM_Block_Set(xStart,xStart+xLong-1,yStart,yStart+yLong-1);
        WriteComm(0x2c);  // ----for continous GRAM write

	for (temp=0; temp<xLong*yLong; temp++)
	{
          WriteDData(Color);
  	 //WriteData(Color>>8);
	 //WriteData(Color&0xff);
	}
*/
}

/* ------------------- show a picture stored in a char* array ------------*/

static void LCD_Fill_Pic(uint16_t x, uint16_t y, uint16_t pic_H, uint16_t pic_V, const unsigned char* pic)
{
        uint32_t i;
//	uint16_t j;

 	WriteComm(0x36); //Set_address_mode
 	WriteData(0x08); // show vertically 
        GRAM_Block_Set(x,x+pic_H-1,y,y+pic_V-1);
        WriteComm(0x2c);  // ----for continous GRAM write
	for (i = 0; i < pic_H*pic_V*2; i+=2)
	{
           WriteData(pic[i]);
           WriteData(pic[i+1]);             	  
 
	}
 	WriteComm(0x36); //Set_address_mode
 	WriteData(0x68); //show horizontally
}


//-------------- prepare LCD -----------------------
void LCD_prepare(void)
{
        LCD_HD_reset();
        LCD_INIT_RM68140();
        WriteComm(0x3a);WriteData(0x66); //set pixel format 18bits pixel
        WriteComm(0x36);WriteData(0x00); //R-G-B set color data order same as BMP file
// SET ENTRY MODE, FLIP MODE HERE IF NECESSARY
//BRIGHTNESS CONTROL HERE IF NECESSARY
}

//------------- load BMP file from user space and transmit to spi to display on LCD ----------------
//  !!! this function is interruptable !!!
int show_user_bmpf(char* str_f)
{
        int i;
//      uint8_t SPIBUFF_SIZE=36;//-- buffsize for one-time SPI transmital. MAX. 32*9=36bytes
        uint8_t buff[8];//---for temp. data buffering
        uint8_t data_buff[SPIBUFF_SIZE];//--temp. data buff for SPI transmit
        loff_t offp; //-- pfile offset position, use long type will case vfs_read() fail
        u32 picWidth,picHeight; //---BMP file width and height
        u32 total; //--total bytes of bmp file
        u16 residual; //--residual after total/
        u16 nbuff; //=total/SPIBUFF_SIZE
        u16 Hs,He,Vs,Ve; //--GRAM area definition parameters
        u16 Hb,Vb; //--GRAM area corner gap distance from coordinate origin
        ssize_t nret;//vfs_read() return value

//      char str_f[]="/tmp/P35.bmp";
        struct file *fp;
        mm_segment_t fs; //virtual address space parameter


        fp=filp_open(str_f,O_RDONLY,0);
        if(IS_ERR(fp))
        {
                printk("Fail to open %s!\n",str_f);
                return -1;
        }
        fs=get_fs(); //retrieve and store virtual address space boundary limit of current process
        set_fs(KERNEL_DS); // set address space limit to that of the kernel (whole of 4GB)
        offp=18; //where bmp width-height data starts
        vfs_read(fp,buff,sizeof(buff),&offp);// offp must be loff_t type!!!  vfs_read() will return 0 for first bytes if offp define$
        picWidth=buff[3]<<24|buff[2]<<16|buff[1]<<8|buff[0];
        picHeight=buff[7]<<24|buff[6]<<16|buff[5]<<8|buff[4];
//        printk("----%s:  picWidth=%d   picHeight=%d -----\n",str_f,picWidth,picHeight);
        if((picWidth > 320) | (picHeight > 480))
        {
//                printk("----- picture size too large -----\n");
                return -1;
        }

        //----------------- calculate GRAM area ---------------------
        Hb=(320-picWidth+1)/2;
        Vb=(480-picHeight+1)/2;
        Hs=Hb;He=Hb+picWidth-1;
        Vs=Vb;Ve=Vb+picHeight-1;
//        printk("Hs=%d  He=%d \n",Hs,He);
//        printk("Vs=%d  Ve=%d \n",Vs,Ve);

        GRAM_Block_Set(Hs,He,Vs,Ve); //--set GRAM area
        WriteComm(0x2c); //--prepare for continous GRAM write

        total=picWidth*picHeight*3; //--total bytes of BGR data,for 24bits BMP file
//        printk("total=%d\n",total);
        nbuff=total/SPIBUFF_SIZE; //-- how many times of SPI transmits needed,with SPIBUFF_SIZE each time.
//        printk("nbuff=%d\n",nbuff);
        residual=total%SPIBUFF_SIZE; //--residual data 

        printk("--------------------- Start show_user_bmpf() SPI transmission --------------------\n");
        offp=54; //--offset where BGR data begins

        //-------------------------- SPI transmit data to LCD  ---------------------
        for(i=0;i<nbuff;i++)
        {
                nret=vfs_read(fp,data_buff,sizeof(data_buff),&offp);// offp must be loff_t type!!!  vfs_read() will return 0 for fir$
                //printk("nret=%d\n",nret);
                WriteNData(data_buff,SPIBUFF_SIZE);
                //offp+=SPIBUFF_SIZE;
        }
        //for(i=0;i<36;i++)printk("data_buff[%d]: 0x%0x\n",i,data_buff[i]);
//        printk("----------- Start transmit residual data %d bytes ------------\n",residual);
        if(residual!=0)
        {
                vfs_read(fp,data_buff,residual,&offp);// offp must be loff_t type!!!  vfs_read() will return 0 for first bytes if of$
                WriteNData(data_buff,residual);
        }
       printk("------------------  Finish show_user_bmpf() SPI transmission  -----------------\n");

        filp_close(fp,NULL);
        set_fs(fs);//reset address space limit to the original one
        return 0;
}



//--------- load bmp file color data to mem buff  -------------- 
//--------- and return pointer to the buff       ---------------
unsigned char* load_user_bmpf(const char* str_f)
{
        uint8_t buff[8];//---for temp. data buffering
        loff_t offp; //-- pfile offset position, use long type will case vfs_read() fail
        u32 picWidth,picHeight; //---BMP file width and height
        u32 total; //--total bytes of bmp file
        u16 Hs,He,Vs,Ve; //--GRAM area definition parameters
        u16 Hb,Vb; //--GRAM area corner gap distance from coordinate origin
        ssize_t nret;//vfs_read() return value
        struct file *fp;
        unsigned char* pmem_color_data=NULL;
        mm_segment_t fs; //virtual address space parameter

//-------- allocate mem for color data ----------
        pmem_color_data=(unsigned char*)vmalloc(COLOR_DATA_SIZE);
        if(pmem_color_data <0)
        {
                printk("------- fail to vmalloc mem for color data! ------\n");
                return NULL;
        }
        else
                printk("------- vmalloc mem for color data successfully! ------\n"); 

//--------- open file and load data to mem ------------
        fp=filp_open(str_f,O_RDONLY,0);
        if(IS_ERR(fp))
        {
                printk("Fail to open %s!\n",str_f);
                return -1;
        }
        fs=get_fs(); //retrieve and store virtual address space boundary limit of current process
        set_fs(KERNEL_DS); // set address space limit to that of the kernel (whole of 4GB)

        offp=18; //where bmp width-height data starts
        vfs_read(fp,buff,sizeof(buff),&offp);// offp must be loff_t type!!!  vfs_read() will return 0 for first bytes if offp defined$
        picWidth=buff[3]<<24|buff[2]<<16|buff[1]<<8|buff[0];
        picHeight=buff[7]<<24|buff[6]<<16|buff[5]<<8|buff[4];
        printk("----%s:  picWidth=%d   picHeight=%d -----\n",str_f,picWidth,picHeight);
        if((picWidth > 320) | (picHeight > 480))
        {
                printk("----- picture size too large -----\n");
                return -1;
        }

        //----------------- calculate GRAM area ---------------------
        //---picWidth and picHeight shall be multiples of 4
        Hb=(320-picWidth+1)/2;
        Vb=(480-picHeight+1)/2;
        Hs=Hb;He=Hb+picWidth-1;
        Vs=Vb;Ve=Vb+picHeight-1;
        printk("Hs=%d  He=%d \n",Hs,He);
        printk("Vs=%d  Ve=%d \n",Vs,Ve);

        total=picWidth*picHeight*3; //--total bytes of BGR data,for 24bits BMP file
        printk("total=%d\n",total);

        offp=54; //--offset where BGR data begins
        //----------------  copy data to buff ----------------
        nret=vfs_read(fp,pmem_color_data,total,&offp);// offp must be loff_t type!!!  vfs_read() will return 0 for first bytes if off$
        printk("--------- vfs_read() %d bytes data, while actual total is %d bytes -----------\n",nret,total);

        filp_close(fp,NULL);
        set_fs(fs);//reset address space limit to the original one

        return pmem_color_data;
}


//------------------------------- spi transmit data in mem buff ---------------
//mem_data:pointer to data         total : total bytes to be transmitted
static int display_full(unsigned const char* data_buff)
{
        int ret=0;
        int i;
        u32 total=480*320*3; //total bytes for a full size image
        u16 residual; //--residual after total/
        u16 nbuff; //=total/SPIBUFF_SIZE

        nbuff=total/SPIBUFF_SIZE;
        residual=total%SPIBUFF_SIZE;

        GRAM_Block_Set(0,319,0,479); //--set GRAM area,full size
        WriteComm(0x2c); //--prepare for continous GRAM write

        printk("--------------------- Start displaying mem data --------------------\n");
        //-------------------------- SPI transmit data to LCD  ---------------------
        for(i=0;i<nbuff;i++)
        {
                WriteNData(data_buff,SPIBUFF_SIZE);
                data_buff+=SPIBUFF_SIZE;
        }
        if(residual!=0)
        {
                WriteNData(data_buff,residual);
        }
        printk("--------------------- Finish displaying mem data --------------------\n");
        return ret;
}


//------------ spi transmit data in mem buff by spi_trans_block_halfduplex() ---------------
//static int spi_trans_block_halfduplex(struct base_spi *m, const char *pdata,long ndat)
static int display_block_full(unsigned const char* data_buff)
{
        int ret=0;
        u32 total=480*320*3; //total bytes for a full size image

        GRAM_Block_Set(0,319,0,479); //--set GRAM area,full size
        WriteComm(0x2c); //--prepare for continous GRAM write

        printk("--------------------- Start trans by block_halfduplex()  --------------------\n");
        //-------------------------- SPI transmit data to LCD  ---------------------
        DCXdata;
        spi_trans_block_halfduplex(&spi_LCD,data_buff,total);
        printk("--------------------- Finish trans by block_halfduplex() --------------------\n");
        return ret;
}


#endif




