/*--------------- With referecne to    WWW.RFinCHINA.COM    ----------------
TODOs and BUGs:
High cpu usage ~11%

 lib: -lm
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cc1101_spi.h"

//DRIVER:   ./build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29/drivers/spi/spidev.c
//DRIVER:   ./build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29/drivers/spi/spi.c

#define DATA_LENGTH 64 //define max data length,including 1byte data-length, 1byte dest-addr,2bytes appends,if applied. +other data MUST be less than SPI shot data
#define CC1101_FXOSC 26  //MHz

#define WRITE_SINGLE 0x00
#define WRITE_BURST 0x40
#define READ_SINGLE 0x80
#define READ_BURST  0xC0
#define BYTES_IN_RXFIFO 0x7F // effective bits for RXFIFO: 0b0111 1111
#define CRC_OK      0x80

//----first 4bits of Chip Status
#define STATUS_IDLE 0
#define STATUS_RX 1
#define STATUS_TX 2
#define STATUS_FSTXON 3
#define STATUS_CALIBRATE 4
#define STATUS_SETTLING 5
#define RXFIFO_OVERFLOW 6
#define TXFIFO_UNDERFLOW 7

//---Message String --
int num_FuncMessg=0;// 0---STATUS=0x1F trap
char str_FuncMessg[20][300];
char str_TmpMessg[300];
time_t tnow;
struct tm *tm_local;


//---Modulation Format---
enum mod_fmt \
{
MODFMT_2_FSK=0,
MODFMT_GFSK=1,
MODFMT_ASK_OOK=3,
MODFMT_4_FSK=4,
MODFMT_MSK=7
};

//------------------ PA settingup ------------
//uint8_t PaTabel[8] = {0x12 ,0x12 ,0x12 ,0x12 ,0x12,0x12 ,0x12 ,0x12};     //-30dBm
//uint8_t PaTabel[8] = {0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60};      //0dBm
uint8_t PaTabel[8] = {0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0};   //10dBm

//--------------------------------
//char    flag,m;
//#define TxRxBuf_Len 32
//char    TxBuf[TxRxBuf_Len];  
//char    RxBuf[TxRxBuf_Len];

uint8_t  decRSSI; // RSSI valueֵ

//----------------- function declaration -----------------
//void SpiInit(void);
//void CpuInit(void);
void RESET_CC1100(void);
//void POWER_UP_RESET_CC1100(void);
uint8_t halSpiGetStatus(void);
void halSpiWriteReg(uint8_t addr, uint8_t value);
void halSpiWriteBurstReg(uint8_t addr, uint8_t *buffer, uint8_t count);
void halSpiStrobe(uint8_t strobe);
uint8_t halSpiReadReg(uint8_t addr);
void halSpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t count);
uint8_t halSpiReadStatus(uint8_t addr);
void halRfWriteRfSettings(void);
void setAddress(uint8_t desAddr);// set chip addr.
void disAddrFilt(void);//disable address filter
void enAddrFilt(void);//enable address filter
char* getModFmtStr(void);
void setModFmt(enum mod_fmt mod);
void setKbitRateME(uint8_t rate_m, uint8_t rate_e);
float getKbitRateMHz(void);
void setCarFreqMHz(float fMHz, uint8_t chan);
float getCarFreqMHz(void);
void setFreqDeviatME(uint8_t m, uint8_t e);
float getFreqDeviatKHz(void);
float getIfFreqKHz(void);
void setChanBWME(uint8_t m, uint8_t e);
float getChanBWKHz(void);
void setChanSpcME(uint8_t m, uint8_t e);
float getChanSpcKHz(void);
int readRSSIdbm(void);
int  getRSSIdbm(void);
void halRfSendPacket(uint8_t *txBuffer, uint8_t size, uint8_t desAddr); 
uint8_t halRfReceivePacket(uint8_t *rxBuffer, uint8_t length);  
//void UART_init();
//void R_S_Byte(char R_Byte);

//-------------- define registers --------------
// CC1100 STROBE, CONTROL AND STATUS REGSITER
#define CCxxx0_IOCFG2       0x00        // GDO2 output pin configuration
#define CCxxx0_IOCFG1       0x01        // GDO1 output pin configuration
#define CCxxx0_IOCFG0       0x02        // GDO0 output pin configuration
#define CCxxx0_FIFOTHR      0x03        // RX FIFO and TX FIFO thresholds
#define CCxxx0_SYNC1        0x04        // Sync word, high uint8_t
#define CCxxx0_SYNC0        0x05        // Sync word, low uint8_t
#define CCxxx0_PKTLEN       0x06        // Packet length
#define CCxxx0_PKTCTRL1     0x07        // Packet automation control
#define CCxxx0_PKTCTRL0     0x08        // Packet automation control
#define CCxxx0_ADDR         0x09        // Device address
#define CCxxx0_CHANNR       0x0A        // Channel number
#define CCxxx0_FSCTRL1      0x0B        // Frequency synthesizer control
#define CCxxx0_FSCTRL0      0x0C        // Frequency synthesizer control
#define CCxxx0_FREQ2        0x0D        // Frequency control word, high uint8_t
#define CCxxx0_FREQ1        0x0E        // Frequency control word, middle uint8_t
#define CCxxx0_FREQ0        0x0F        // Frequency control word, low uint8_t
#define CCxxx0_MDMCFG4      0x10        // Modem configuration
#define CCxxx0_MDMCFG3      0x11        // Modem configuration
#define CCxxx0_MDMCFG2      0x12        // Modem configuration
#define CCxxx0_MDMCFG1      0x13        // Modem configuration
#define CCxxx0_MDMCFG0      0x14        // Modem configuration
#define CCxxx0_DEVIATN      0x15        // Modem deviation setting
#define CCxxx0_MCSM2        0x16        // Main Radio Control State Machine configuration
#define CCxxx0_MCSM1        0x17        // Main Radio Control State Machine configuration
#define CCxxx0_MCSM0        0x18        // Main Radio Control State Machine configuration
#define CCxxx0_FOCCFG       0x19        // Frequency Offset Compensation configuration
#define CCxxx0_BSCFG        0x1A        // Bit Synchronization configuration
#define CCxxx0_AGCCTRL2     0x1B        // AGC control
#define CCxxx0_AGCCTRL1     0x1C        // AGC control
#define CCxxx0_AGCCTRL0     0x1D        // AGC control
#define CCxxx0_WOREVT1      0x1E        // High uint8_t Event 0 timeout
#define CCxxx0_WOREVT0      0x1F        // Low uint8_t Event 0 timeout
#define CCxxx0_WORCTRL      0x20        // Wake On Radio control
#define CCxxx0_FREND1       0x21        // Front end RX configuration  
#define CCxxx0_FREND0       0x22        // Front end TX configuration
#define CCxxx0_FSCAL3       0x23        // Frequency synthesizer calibration
#define CCxxx0_FSCAL2       0x24        // Frequency synthesizer calibration
#define CCxxx0_FSCAL1       0x25        // Frequency synthesizer calibration
#define CCxxx0_FSCAL0       0x26        // Frequency synthesizer calibration
#define CCxxx0_RCCTRL1      0x27        // RC oscillator configuration
#define CCxxx0_RCCTRL0      0x28        // RC oscillator configuration
#define CCxxx0_FSTEST       0x29        // Frequency synthesizer calibration control
#define CCxxx0_PTEST        0x2A        // Production test
#define CCxxx0_AGCTEST      0x2B        // AGC test
#define CCxxx0_TEST2        0x2C        // Various test settings
#define CCxxx0_TEST1        0x2D        // Various test settings
#define CCxxx0_TEST0        0x2E        // Various test settings
// Strobe commands
#define CCxxx0_SRES         0x30        // Reset chip.
#define CCxxx0_SFSTXON      0x31        // Enable and calibrate frequency synthesizer (if MCSM0.FS_AUTOCAL=1).
                                        // If in RX/TX: Go to a wait state where only the synthesizer is
                                        // running (for quick RX / TX turnaround).
#define CCxxx0_SXOFF        0x32        // Turn off crystal oscillator.
#define CCxxx0_SCAL         0x33        // Calibrate frequency synthesizer and turn it off
                                        // (enables quick start).
#define CCxxx0_SRX          0x34        // Enable RX. Perform calibration first if coming from IDLE and
                                        // MCSM0.FS_AUTOCAL=1.
#define CCxxx0_STX          0x35        // In IDLE state: Enable TX. Perform calibration first if
                                        // MCSM0.FS_AUTOCAL=1. If in RX state and CCA is enabled:
                                        // Only go to TX if channel is clear.
#define CCxxx0_SIDLE        0x36        // Exit RX / TX, turn off frequency synthesizer and exit
                                        // Wake-On-Radio mode if applicable.
#define CCxxx0_SAFC         0x37        // Perform AFC adjustment of the frequency synthesizer
#define CCxxx0_SWOR         0x38        // Start automatic RX polling sequence (Wake-on-Radio)
#define CCxxx0_SPWD         0x39        // Enter power down mode when CSn goes high.
#define CCxxx0_SFRX         0x3A        // Flush the RX FIFO buffer.
#define CCxxx0_SFTX         0x3B        // Flush the TX FIFO buffer.
#define CCxxx0_SWORRST      0x3C        // Reset real time clock.
#define CCxxx0_SNOP         0x3D        // No operation. May be used to pad strobe commands to two
                                        // uint8_ts for simpler software.
#define CCxxx0_PARTNUM      0x30
#define CCxxx0_VERSION      0x31
#define CCxxx0_FREQEST      0x32
#define CCxxx0_LQI          0x33
#define CCxxx0_RSSI         0x34
#define CCxxx0_MARCSTATE    0x35
#define CCxxx0_WORTIME1     0x36
#define CCxxx0_WORTIME0     0x37
#define CCxxx0_PKTSTATUS    0x38
#define CCxxx0_VCO_VC_DAC   0x39
#define CCxxx0_TXBYTES      0x3A
#define CCxxx0_RXBYTES      0x3B
#define CCxxx0_PATABLE      0x3E
#define CCxxx0_TXFIFO       0x3F
#define CCxxx0_RXFIFO       0x3F

//-------  RF_SETTINGS is a data structure which contains all relevant CCxxx0 registers
typedef struct S_RF_SETTINGS
{
    uint8_t FSCTRL2;   //-----aditional ????
    uint8_t FSCTRL1;   // Frequency synthesizer control.
    uint8_t FSCTRL0;   // Frequency synthesizer control.
    uint8_t FREQ2;     // Frequency control word, high uint8_t.
    uint8_t FREQ1;     // Frequency control word, middle uint8_t.
    uint8_t FREQ0;     // Frequency control word, low uint8_t.
    uint8_t MDMCFG4;   // Modem configuration.
    uint8_t MDMCFG3;   // Modem configuration.
    uint8_t MDMCFG2;   // Modem configuration.
    uint8_t MDMCFG1;   // Modem configuration.
    uint8_t MDMCFG0;   // Modem configuration.
    uint8_t CHANNR;    // Channel number.
    uint8_t DEVIATN;   // Modem deviation setting (when FSK modulation is enabled).
    uint8_t FREND1;    // Front end RX configuration.
    uint8_t FREND0;    // Front end RX configuration.
    uint8_t MCSM0;     // Main Radio Control State Machine configuration.
    uint8_t FOCCFG;    // Frequency Offset Compensation Configuration.
    uint8_t BSCFG;     // Bit synchronization Configuration.
    uint8_t AGCCTRL2;  // AGC control.
    uint8_t AGCCTRL1;  // AGC control.
    uint8_t AGCCTRL0;  // AGC control.
    uint8_t FSCAL3;    // Frequency synthesizer calibration.
    uint8_t FSCAL2;    // Frequency synthesizer calibration.
    uint8_t FSCAL1;    // Frequency synthesizer calibration.
    uint8_t FSCAL0;    // Frequency synthesizer calibration.
    uint8_t FSTEST;    // Frequency synthesizer calibration control
    uint8_t TEST2;     // Various test settings.
    uint8_t TEST1;     // Various test settings.
    uint8_t TEST0;     // Various test settings.
    uint8_t IOCFG2;    // GDO2 output pin configuration
    uint8_t IOCFG0;    // GDO0 output pin configuration
    uint8_t PKTCTRL1;  // Packet automation control.
    uint8_t PKTCTRL0;  // Packet automation control.
    uint8_t ADDR;      // Device address.
    uint8_t PKTLEN;    // Packet length.
} RF_SETTINGS;

//--------------- Assignment for RF_SETTINGS --------------
 RF_SETTINGS rfSettings =
{
    0x00,
    0x06,   //- FSCTRL1   Frequency synthesizer control.
    0x00,   //- FSCTRL0   Frequency synthesizer control.
    0x10,   //- FREQ2     Frequency control word, high byte.
    0xA7,   //- FREQ1     Frequency control word, middle byte.
    0x62,   //- FREQ0     Frequency control word, low byte.
    0xFA,   //- MDMCFG4   Modem configuration.2.4k:0xF6 5k:0xF7 10k:0xF8 50k:FA 100k:0xFB  [3:0]-DRATE_E 6 
    0xF8,   //- MDMCFG3   Modem configuration.2.4k:0x83 5k:0x93 10k:0x93 50k:F8 100k:0xF8  [7:0]-DRATE_M=131
    0x13,   //- MDMCFG2   Modem configuration.//[6:4] 000--2FSK  001---GFSK 011---ASK/OOK 111---MSK
    0x22,   //- MDMCFG1   Modem configuration.  --[1:0]CHANSPC_E ---[7]=1 FEC_EN enable forward correcting
    0xF8,   //- MDMCFG0   Modem configuration.  --CHANSPC_M
    0x00,   //- CHANNR    Channel number.  --CHAN
    0x15,   //- DEVIATN   Modem deviation setting (when FSK modulation is enabled). [2:0]----DEVIATION_M  [6:4]----DEVIATION_E
    0x56,   //- FREND1    Front end RX configuration.
    0x10,   //- FREND0    Front end RX configuration.
    0x18,   //- MCSM0     Main Radio Control State Machine configuration.
    0x16,   //- FOCCFG    Frequency Offset Compensation Configuration.
    0x6C,   //- BSCFG     Bit synchronization Configuration.
    0x03,   //- AGCCTRL2  AGC control.
    0x40,   //- AGCCTRL1  AGC control.
    0x91,   //- AGCCTRL0  AGC control.
    0xE9,   //- FSCAL3    Frequency synthesizer calibration.
    0x2A,   //- FSCAL2    Frequency synthesizer calibration.
    0x00,   //- FSCAL1    Frequency synthesizer calibration.
    0x1F,   //- FSCAL0    Frequency synthesizer calibration.
    0x59,   //- FSTEST    Frequency synthesizer calibration.
    0x81,   //- TEST2     Various test settings.
    0x35,   //- TEST1     Various test settings.
    0x09,   //- TEST0     Various test settings.
    0x0B,   // IOCFG2    GDO2 output pin configuration.
    0x06,   // IOCFG0   GDO0 output pin configuration. Refer to SmartRF?Studio User Manual for detailed pseudo register explanation.
    0x07,   // PKTCTRL1  Packet automation control. --[2]=1 enable APPEND_STATUS(RSSI+LQI) and CRC [1:0]=3,address check and 0/255 broadcast
    0x45,   //- PKTCTRL0  Packet automation control. --[6]=1 turn data whitening on.
    0x00,   //- ADDR      Device address. 
    0x40    //- PKTLEN    Packet length.variable packet lenth mode, Max packet len = 60
};

//------------------------function definition------------------------------ 
void halSpiStrobe(uint8_t strobe)
{
	SPI_Write(&strobe,1);
}

void RESET_CC1100(void)
{
	halSpiStrobe(CCxxx0_SRES);
}

/* read Status byte from MISO */
uint8_t halSpiGetStatus(void)
{
	uint8_t status;
        SPI_Read(&status,1);
	return status;
}

/* Write to a register */
void halSpiWriteReg(uint8_t addr, uint8_t value)
{
	uint8_t data[2];
	data[0]=addr; data[1]=value;
	SPI_Write(data,2);
}

/*  burst write to registers */
void halSpiWriteBurstReg(uint8_t addr, uint8_t *buffer, uint8_t count)
{
	uint8_t tmp;
	tmp=addr|WRITE_BURST;
	SPI_Write_then_Write(&tmp,1,buffer,count);
}

/* read a register */
uint8_t halSpiReadReg(uint8_t addr)
{
	uint8_t tmp,value;
	tmp=addr|READ_SINGLE;
	SPI_Write_then_Read(&tmp,1,&value,1);
	return value;
}

/* burst read registers */
//--- Max. count=31 ---//
void halSpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t count)
{
	uint8_t tmp,value;
        tmp=addr|READ_BURST;
        SPI_Write_then_Read(&tmp,1,buffer,count);
}

/* read a status register */
uint8_t halSpiReadStatus(uint8_t addr)
{
	uint8_t tmp,value;
	tmp=addr|READ_BURST; //-- To read a status-register with command header 11
	SPI_Write_then_Read(&tmp,1,&value,1);
	return value;
}


// --------- configure registers ------------
void halRfWriteRfSettings(void) 
{
//-- registers configuration
    halSpiWriteReg(CCxxx0_FSCTRL0,  rfSettings.FSCTRL2); //additionl;
    halSpiWriteReg(CCxxx0_FSCTRL1,  rfSettings.FSCTRL1);
    halSpiWriteReg(CCxxx0_FSCTRL0,  rfSettings.FSCTRL0);
    halSpiWriteReg(CCxxx0_FREQ2,    rfSettings.FREQ2);
    halSpiWriteReg(CCxxx0_FREQ1,    rfSettings.FREQ1);
    halSpiWriteReg(CCxxx0_FREQ0,    rfSettings.FREQ0);
    halSpiWriteReg(CCxxx0_MDMCFG4,  rfSettings.MDMCFG4);
    halSpiWriteReg(CCxxx0_MDMCFG3,  rfSettings.MDMCFG3);
    halSpiWriteReg(CCxxx0_MDMCFG2,  rfSettings.MDMCFG2);
    halSpiWriteReg(CCxxx0_MDMCFG1,  rfSettings.MDMCFG1);
    halSpiWriteReg(CCxxx0_MDMCFG0,  rfSettings.MDMCFG0);
    halSpiWriteReg(CCxxx0_CHANNR,   rfSettings.CHANNR);
    halSpiWriteReg(CCxxx0_DEVIATN,  rfSettings.DEVIATN);
    halSpiWriteReg(CCxxx0_FREND1,   rfSettings.FREND1);
    halSpiWriteReg(CCxxx0_FREND0,   rfSettings.FREND0);
    halSpiWriteReg(CCxxx0_MCSM0 ,   rfSettings.MCSM0 );
    halSpiWriteReg(CCxxx0_FOCCFG,   rfSettings.FOCCFG);
    halSpiWriteReg(CCxxx0_BSCFG,    rfSettings.BSCFG);
    halSpiWriteReg(CCxxx0_AGCCTRL2, rfSettings.AGCCTRL2);
    halSpiWriteReg(CCxxx0_AGCCTRL1, rfSettings.AGCCTRL1);
    halSpiWriteReg(CCxxx0_AGCCTRL0, rfSettings.AGCCTRL0);
    halSpiWriteReg(CCxxx0_FSCAL3,   rfSettings.FSCAL3);
    halSpiWriteReg(CCxxx0_FSCAL2,   rfSettings.FSCAL2);
    halSpiWriteReg(CCxxx0_FSCAL1,   rfSettings.FSCAL1);
    halSpiWriteReg(CCxxx0_FSCAL0,   rfSettings.FSCAL0);
    halSpiWriteReg(CCxxx0_FSTEST,   rfSettings.FSTEST);
    halSpiWriteReg(CCxxx0_TEST2,    rfSettings.TEST2);
    halSpiWriteReg(CCxxx0_TEST1,    rfSettings.TEST1);
    halSpiWriteReg(CCxxx0_TEST0,    rfSettings.TEST0);
    halSpiWriteReg(CCxxx0_IOCFG2,   rfSettings.IOCFG2);
    halSpiWriteReg(CCxxx0_IOCFG0,   rfSettings.IOCFG0);
    halSpiWriteReg(CCxxx0_PKTCTRL1, rfSettings.PKTCTRL1);
    halSpiWriteReg(CCxxx0_PKTCTRL0, rfSettings.PKTCTRL0);
    halSpiWriteReg(CCxxx0_ADDR,     rfSettings.ADDR);
    halSpiWriteReg(CCxxx0_PKTLEN,   rfSettings.PKTLEN);
}

//----- set symbol bit rate ------
void setKbitRateME(uint8_t m, uint8_t e)
{
  if(e>15)
  {
	printf(" E value is NOT valid!\n");
	return;
  }
  rfSettings.MDMCFG4&=0xf0;
  rfSettings.MDMCFG4|=e;
  rfSettings.MDMCFG3=m;

  halSpiWriteReg(CCxxx0_MDMCFG4,rfSettings.MDMCFG4);
  halSpiWriteReg(CCxxx0_MDMCFG3,rfSettings.MDMCFG3);

}


//------ get symbol bit rate -------
float getKbitRate(void)
{
  float brate; // symbol kbit rate 
  uint8_t drate_m,drate_e;

  drate_m=halSpiReadReg(CCxxx0_MDMCFG3);
  drate_e=halSpiReadReg(CCxxx0_MDMCFG4)&0x0f;
  brate=(256+drate_m)*pow(2,drate_e-28)*CC1101_FXOSC*1000; //Kbits/s
  
  return brate;
}


//----------- set carrier frequency (MHZ) -----
// fMHz -- base frequency
// channr -- channel number
void setCarFreqMHz(float fMHz, uint8_t channr)  
{
	uint8_t fr0,fr1,fr2;
	int FREQ=fMHz*pow(2,16)/CC1101_FXOSC;
	fr2=FREQ>>16;
	fr1=(FREQ&0xff00)>>8;
	fr0=FREQ&0xff;

	rfSettings.FREQ2=fr2;
	rfSettings.FREQ1=fr1;
	rfSettings.FREQ0=fr0;
	rfSettings.CHANNR=channr;

	halSpiWriteReg(CCxxx0_FREQ2,rfSettings.FREQ2);
	halSpiWriteReg(CCxxx0_FREQ1,rfSettings.FREQ1);
	halSpiWriteReg(CCxxx0_FREQ0,rfSettings.FREQ0);
	halSpiWriteReg(CCxxx0_CHANNR,rfSettings.CHANNR);
}

//----------- get carrier frequency (MHz) -------
float getCarFreqMHz(void)
{
   float carfreq;
   uint8_t freq0,freq1,freq2;
   int freq;
   uint8_t chanspc_e,chanspc_m;
   uint8_t channel;

    freq0=halSpiReadReg(CCxxx0_FREQ0);
    freq1=halSpiReadReg(CCxxx0_FREQ1);
    freq2=halSpiReadReg(CCxxx0_FREQ2);
    freq=(freq2<<16)+(freq1<<8)+freq0;
    chanspc_e=halSpiReadReg(CCxxx0_MDMCFG1)&0x03;
    chanspc_m=halSpiReadReg(CCxxx0_MDMCFG0);
    channel=halSpiReadReg(CCxxx0_CHANNR);

    carfreq=CC1101_FXOSC/pow(2,16)*(freq+channel*(256+chanspc_m*pow(2,chanspc_e-2)));

   return carfreq;
}


//------- st frequency deviation(KHz) --------
void setFreqDeviatME(uint8_t m, uint8_t e)
{
  //---  apply to 2-FST/GFS/4-FSK ref. to manual for detail
  if(m>7 || e>7)
  {
	printf("M or E value is NOT valid!\n");
	return;
  }
  rfSettings.DEVIATN&=0x8f;
  rfSettings.DEVIATN|=(e<<4);
  rfSettings.DEVIATN&=0xf8;
  rfSettings.DEVIATN|=m;

  halSpiWriteReg(CCxxx0_DEVIATN,rfSettings.DEVIATN);
} 

//------- get frequency deviation(KHz) --------
float getFreqDeviatKHz(void)
{
   uint8_t deviat_e,deviat_m;
   float dev;

   deviat_e=(halSpiReadReg(CCxxx0_DEVIATN)&0x70)>>4;
   deviat_m=halSpiReadReg(CCxxx0_DEVIATN)&0x07;
   dev=CC1101_FXOSC/pow(2,17)*(8+deviat_m)*pow(2,deviat_e)*1000;

   return dev;
}

//--------  set chip address -------
void setAddress(uint8_t desAddr)
{
  rfSettings.ADDR=desAddr;
  halSpiWriteReg(CCxxx0_ADDR,rfSettings.ADDR);
}
//--------  disable auto. address filter -----
void disAddrFilt(void)
{
  rfSettings.PKTCTRL1&=0xFC; //PKTCTRL1[1:0]=0X00 --No address check
  halSpiWriteReg(CCxxx0_PKTCTRL1,rfSettings.PKTCTRL1);  
}

//---------  enable auto. address filter  -----
// enable address check and treat 0x00 and 0xFF as broadcast
// it will discard data with invalid dest-address and restart RX_MODE
void enAddrFilt(void)
{
  rfSettings.PKTCTRL1|=0x03; //PKTCTRL1[1:0]=0X11  enable address check and treat 0x00 and 0xFF as broadcast
  halSpiWriteReg(CCxxx0_PKTCTRL1,rfSettings.PKTCTRL1);  
}


//------ get Mode Format -----------
char* getModFmtStr(void)
{
	char* str;
	char *str_2FSK="2-FSK";  // Do not use str_2FSK[]="...".
	char *str_GFSK="GFSK";
	char *str_ASKOOK="ASK/OOK";
	char *str_4FSK="4-FSK";
	char *str_MSK="MSK";
	char *str_UNKNOWN="UNKNOWN";

	uint8_t mod;
	mod=(halSpiReadReg(CCxxx0_MDMCFG2)&0x70)>>4;

	switch(mod){
		case MODFMT_2_FSK: str=str_2FSK;break;
		case MODFMT_GFSK: str=str_GFSK;break;
		case MODFMT_ASK_OOK: str=str_ASKOOK;break;
		case MODFMT_4_FSK: str=str_4FSK;break;
		case MODFMT_MSK: str=str_MSK;break;
		default: str=str_UNKNOWN; 
	}
	return str;
}

//------ st Modulation format -------
void setModFmt(enum mod_fmt mod)
{
  rfSettings.MDMCFG2&=0x8F;
  rfSettings.MDMCFG2|=(mod<<4);

  halSpiWriteReg(CCxxx0_MDMCFG2,rfSettings.MDMCFG2);
}




//------ get intermediate frequency ------
float getIfFreqKHz(void)
{
	float iffreq;
	uint8_t fsctrl1_freq;
	fsctrl1_freq=halSpiReadReg(CCxxx0_FSCTRL1)&0x1f;
	iffreq=CC1101_FXOSC/pow(2,10)*fsctrl1_freq*1000;

	return iffreq;
}

//---------- set filter bandwith ------
void setChanBWME(uint8_t m, uint8_t e)
{
  if(m>3 || e>3)
  {
	printf("M or E value is NOT valid!\n");
	return;
  }
  rfSettings.MDMCFG4&=0xCF;
  rfSettings.MDMCFG4|=(m<<4);
  rfSettings.MDMCFG4&=0x3F;
  rfSettings.MDMCFG4|=(e<<6);

  halSpiWriteReg(CCxxx0_MDMCFG4,rfSettings.MDMCFG4);


}


//----------- get channel filter bandwith KHz -----
float getChanBWKHz(void)
{
	float chanBW;
	uint8_t chanBW_E,chanBW_M;
	chanBW_E=halSpiReadReg(CCxxx0_MDMCFG4)>>6;
	chanBW_M=(halSpiReadReg(CCxxx0_MDMCFG4)&0x30)>>4;
	chanBW=CC1101_FXOSC/(8*(4+chanBW_M)*pow(2,chanBW_E))*1000;

	return chanBW;
}

//--------- set channel spacing ---------
void setChanSpcME(uint8_t m, uint8_t e)
{
  if(e>3)
  {
	printf("Value CHANSPC_E is not valid!\n");
	return;
  }
  rfSettings.MDMCFG1&=0xfc;
  rfSettings.MDMCFG1|=e;
  rfSettings.MDMCFG0=m;

  halSpiWriteReg(CCxxx0_MDMCFG0,rfSettings.MDMCFG0);
  halSpiWriteReg(CCxxx0_MDMCFG1,rfSettings.MDMCFG1);
}

//--------- get channel spacing ---------
float getChanSpcKHz(void)
{
   float chanspc;
   uint8_t chanspc_e,chanspc_m;

    chanspc_e=halSpiReadReg(CCxxx0_MDMCFG1)&0x03;
    chanspc_m=halSpiReadReg(CCxxx0_MDMCFG0);

   chanspc=CC1101_FXOSC/pow(2,18)*(256+chanspc_m)*pow(2,chanspc_e)*1000;
   return chanspc;
}



//---- get RSSI(dbm) from RSSI register ---
int readRSSIdbm(void)
{
   int dbmRSSI,ret=0;

   dbmRSSI=halSpiReadStatus(CCxxx0_RSSI);
   if(dbmRSSI >= 128)
	ret=(dbmRSSI-256)/2-74;
   else
    	ret=dbmRSSI/2-74;

   return ret;
}

//---- get RSSI (dbm) from RXFIFO append -----
int getRSSIdbm()
{
   int dbmRSSI;
   if(decRSSI >= 128)
	dbmRSSI=(decRSSI-256)/2-74;
   else
    	dbmRSSI=decRSSI/2-74;

   return dbmRSSI;
}




//----------- transmit  data packet ---------------------
// desAddr -- destination cc1101 address,default =0 broadcast
// size --- payload data length, excluding data-length and dest.-Addr
void halRfSendPacket(uint8_t *txBuffer, uint8_t size, uint8_t desAddr) 
{
    int i,k;
    uint8_t tmp_len;
    uint8_t status,status_state;
    //---timer---
    struct timeval t_start,t_end;
    long cost_time;

    //---- start timer------
    gettimeofday(&t_start,NULL);

    if( size > (DATA_LENGTH-3))//1byte--payload length, 2bytes-appends
     {
	printf("Max. Payload for TXFIFO is %d Bytes!\n",DATA_LENGTH-3);
	return;
     }

    halSpiWriteReg(CCxxx0_TXFIFO, size+1);//!!! size+1(desAddr) --write payload-length(include desAddr) to TXFIFO first!
    halSpiWriteReg(CCxxx0_TXFIFO, desAddr);// --write dest. address to TXFIFO
    tmp_len=size;
    while(tmp_len > 35) //-- spi-send MAX. 1+35 =36bytes each time.
    {
    	halSpiWriteBurstReg(CCxxx0_TXFIFO, txBuffer, 35); //-- spi-send MAX. 1+35 =36bytes each time.
	tmp_len-=35;
	txBuffer+=35;
     }
     if(tmp_len>0)
	 halSpiWriteBurstReg(CCxxx0_TXFIFO, txBuffer,tmp_len); //-- spi-send MAX. 1+35 =36bytes.

    //halSpiStrobe(CCxxx0_SIDLE);
    halSpiStrobe(CCxxx0_STX); //enter transmit mode and send out data

    //------ wait for cc1101 to finish transmittance -----
    //-- you may use GDO0 pin get a finish-transmitting signal ----
      // Wait for GDO0 to be set -> sync transmitted
      //while (!GDO0);
      // Wait for GDO0 to be cleared -> end of packet 
      //while (GDO0);
//    printf("During transmitting...\n");
    status=halSpiGetStatus();
//    printf("STATUS Byte: 0x%02x\n",status);
    status_state=status>>4;
    k=0;
    while(status_state!=STATUS_IDLE)
    {
	//printf("STATUS: 0x%02x\n",status);
	//---if cc1101 corrupt  --
	if((status_state!=STATUS_TX) && (status_state!=STATUS_CALIBRATE) && (status_state!=STATUS_SETTLING))
	{
		printf("STATUS corrupts in TX mode. STATUS=0x%02x\n",status);
		return;
	}
	usleep(100);  //sleep 
	k++;
	if(k>5000)
	{
		printf("halRfSendPacket(): STATUS may corrupts! K exceeds limit while polling status, k=%dn  STATUS=0x%02x\n",k,status);
//		return 0; //fail
	}
	status=halSpiGetStatus();
	status_state=status>>4;
    }
    printf("It takes %dx100us to send out data packet!\n",k);
    //usleep(5000);
//    printf("Before CCxxx0_SFTX RESET, STATUS Byte: 0x%02x\n",halSpiGetStatus());
//    halSpiStrobe(CCxxx0_SFTX);  //flush TXFIFO
//    printf("After CCxxx0_SFTX RESET, STATUS Byte: 0x%02x\n",halSpiGetStatus());
//    usleep(100);
//    printf("100us After CCxxx0_SFTX RESET, STATUS Byte: 0x%02x\n",halSpiGetStatus());

    //--------end timer ------
    gettimeofday(&t_end,NULL);
    cost_time=t_end.tv_usec-t_start.tv_usec;
    if(cost_time<=0)
	    	cost_time=cost_time+(t_end.tv_sec-t_start.tv_sec)*1000000;
    printf("Calling function halRfSendPacket() cost time: %dus\n",cost_time);
}

//-------  set RX Mode -----------------
void setRxMode(void)
{
    halSpiStrobe(CCxxx0_SRX); // set as receiver mode
}

//----------------- Receive Data Packet ---------------
uint8_t halRfReceivePacket(uint8_t *rxBuffer, uint8_t length) // length 
{
    uint8_t ret=0; // 0--fail, 1--sucess, 2--CRC error
    int k;
    uint8_t status,status_state;
    uint8_t desAddr; //dest-address in received packet-data
    uint8_t app_status[2]; //appended status data in received packet
    uint8_t packetLength,tmp_len;
    //------timer -----
    struct timeval t_start,t_end,t_msg;
    long cost_time=0;


    //------start timer-----
    gettimeofday(&t_start,NULL);

    if( length > (DATA_LENGTH-3))//1byte--payload length, 2bytes-appends
     {
	printf("Max. Payload of RXFIFO is %d Bytes!\n",DATA_LENGTH-3);
	return;
     }

   // uint8_t i=length*4;  // to be decided by datarate and length
   //--- !!! Wait a little time just before setting up for next  TX_MODE when you loop funciton halRfReceivePacket().....
    //halSpiStrobe(CCxxx0_SIDLE);
    printf("try halSpiStrobe(CCxxx0_SRX)...\n");
    halSpiStrobe(CCxxx0_SRX);           //enter to RX Mode
   //----after CCxxx0_SRX STATUS changes: CALIBRATE->PPL SETTLING->RX_MODE->IDEL ---------//
 
   //-------!!! you may use GDO0 pin to get a finish-receiving signal -----
/*
        delay(2);
        while (GDO0)  //wait for RX to finish 
        {
                        delay(2);
                        --i;
                        if(i<1)
                    return 0;
           }
*/

//  !!!!!!! High rate SPI polling reduces the RX sensitivity.and a small probality that
//  a single read from registers PKTSTATUS,RXBYTES and TXBYTES is being corrupt !!!!!!!

//------------ METHOD1:  poll STATUS value to know the completion of receiving a packet-------
//------  It works well!! ---------------------
    printf("try halSpiGetStatus() before while ...\n");
    status=halSpiGetStatus();// init the value
    k=0;
    status_state=status>>4;
    while(status_state!=STATUS_IDLE)  // 0x1f ---RX Mode
    {
	//--too fast reading status may cause cc1101 to corrupt
	//---if cc1101 corrupt  --
	if((status_state!=STATUS_RX) && (status_state!=STATUS_CALIBRATE) && (status_state!=STATUS_SETTLING))
	{
		printf("STATUS corrupts in RX mode. STATUS=0x%02x\n",status);
                halSpiStrobe(CCxxx0_SFRX);      //flush RXFIFO
		return 0;
	}

        usleep(500);//!!!CRITICAL!!!!  200~500  try to relief CPU
	k++;
	if(k>5000 && status==0x1F) //  STAUS corrupts in 0x1F  ---test only!!!!
	{
		printf("halRfReceivePacket(): STATUS may corrupts! K exceeds limit while polling status, k=%d  STATUS=0x%02x\n",k,status);
		//------ push up messg to str_FuncMessg[] -------
		if(num_FuncMessg < 20)
		{
                	num_FuncMessg+=1;// 1---STATUS=0x1F trap
			time(&tnow);
			sprintf(str_TmpMessg,"%s",asctime(localtime(&tnow))); //--get formatted time stamp
			sprintf(str_FuncMessg[num_FuncMessg],"%s  --- halRfReceivePacket(): STATUS may corrupts! K exceeds limit while polling status, k=%d  STATUS=0x%02x\n",str_TmpMessg,k,status);
		}
                halSpiStrobe(CCxxx0_SFRX);  //flush RXFIFO
		return 0; //fail
	 }
//        printf("try halSpiGetStatus() in while() ...\n"); 
        status=halSpiGetStatus();
	status_state=status>>4;
//	printf("in while() STATUS: 0x%02x\n",status);
//	if(status>0x10) //--test RXFIFO counting during receiving
//		printf("during while() receiving ...STATUS:0x%02x\n",status);
    }
    printf("RX_MODE: %d*200us idle waiting for RF signal!\n",k);


//------------ METHOD2:  poll PKTSTATUS[0] GDOx value to know the completion of receiving a packet -------
// !!!!!!!! WARNING:  It's easy to corrupt register value during halSpiReadStatus(CCxxx0_PKTSTATUS)  !!!!!!!!!!!!!
/*
    printf("try halSpiReadStatus(..PKTSTATUS) before while ...\n");
    // setting IOCFGx.GDOx_CFG=0x06 to give an interrupt when a complete packet has been received.
    status=halSpiReadStatus(CCxxx0_PKTSTATUS);
    k=0;
    while((status&0x01)!=1)  // GDO0 non-inverted value, 1-- receive completed
    { 
	usleep(500);
        // setting IOCFGx.GDOx_CFG=0x06 to give an interrupt when a complete packet has been received.
        status=halSpiReadStatus(CCxxx0_PKTSTATUS); 
	if(status!=0xa0)
		printf("PKTSTATUS=0x%02x\n",status);
    }
*/

//---------------  METHOD3: GDOx Pin Interrupt  ---------------



//----------- check and read receive data --------
    if ((halSpiReadStatus(CCxxx0_RXBYTES) & BYTES_IN_RXFIFO)) // RXBYTES[7]=1 overflow, if received bytes is valid and not 0
    {
	//---If 2 status bytes(RSSI+(CRC_OK&LQI) appended in packet, then RXBYTES=1(data_length)+data_bytes+2(appends);
//        printf("Received RXBYTES=%d\n",halSpiReadStatus(CCxxx0_RXBYTES));
        packetLength = halSpiReadReg(CCxxx0_RXFIFO);// read out packet-length first
	printf("received packetLenght(include destAddr) =%d bytes\n",packetLength);
        //length = packetLength; // adjust effective data-length,use 
        desAddr = halSpiReadReg(CCxxx0_RXFIFO);// read out desAddr then. The chip will filter it automatically before pushing into RXFIFO.
	printf("desAddr in received packet:0x%02x\n",desAddr);
        if ((packetLength-1) <= length)            //if packet-length-1(desAddr) less than effective data_length
        {
//	    printf("try halSpiReadBurstReg(..rxBuffer..)...\n");
		tmp_len=packetLength-1;//--payload length, 1 byte desAddr deducted
		while(tmp_len>31)//--spi 32BYTE one time only??? 1+packetLengt<=32 ??
		{
             		halSpiReadBurstReg(CCxxx0_RXFIFO, rxBuffer, 31); //read out payload-data, note: 1+packetLength<=32
			rxBuffer+=31;
			tmp_len-=31;
		}
		if(tmp_len>0)
	                halSpiReadBurstReg(CCxxx0_RXFIFO, rxBuffer,tmp_len);

            //--- Read 2 appended status bytes (status[0] = RSSI, status[1] = LQI)
	    //--- if you enable APPEND_STATUS, (RRSI+LQI)+CRC
            halSpiReadBurstReg(CCxxx0_RXFIFO, app_status, 2);
            decRSSI=app_status[0];
//	    printf("try halSpiStrobe(..SFRX..)...\n");
            halSpiStrobe(CCxxx0_SFRX);           //flush RXFIFO

	    //--------end timer ------
	    gettimeofday(&t_end,NULL);
	    cost_time=t_end.tv_usec-t_start.tv_usec;
	    if(cost_time<=0)
	    	cost_time=cost_time+(t_end.tv_sec-t_start.tv_sec)*1000000;
	    printf("Calling function halRfReceivePacket() cost time: %dus\n",cost_time);

            if(app_status[1] & CRC_OK)       //check CRC, CRC_OK=0x80
               return 1; //--1 success
            else
		printf("Received data CRC error!\n");
		return 2; //--2 CRC error
         }
         else
         {
            //length = packetLength;
	    printf("-----Received data length is great than required!\n");
	    //printf("try halSpiStrobe(..SFRX..) in else...\n");
            halSpiStrobe(CCxxx0_SFRX);      //flush RXFIFO
	    return 0; //--0 fail
	 }

    }
    else
	return 0; //--0 receive nothing / fail

}

