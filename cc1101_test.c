#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cc1101_spi.h"

//DRIVER:   ./build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29/drivers/spi/spidev.c
//DRIVER:   ./build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7688/linux-3.18.29/drivers/spi/spi.c

#define WRITE_SINGLE 0x00
#define WRITE_BURST 0x40
#define READ_SINGLE 0x80
#define READ_BURST  0xC0
#define BYTES_IN_RXFIFO 0x7F
#define CRC_OK      0x80

//------------------ PA settingup ------------
//uint8_t PaTabel[8] = {0x12 ,0x12 ,0x12 ,0x12 ,0x12,0x12 ,0x12 ,0x12};     //-30dBm   ������С
//uint8_t PaTabel[8] = {0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60};      //0dBm
 uint8_t PaTabel[8] = {0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0 ,0xC0};   //10dBm     ��������

//--------------------------------
char    flag,m;
#define TxRxBuf_Len 32
char    TxBuf[TxRxBuf_Len];  
char    RxBuf[TxRxBuf_Len];

uint8_t  decRSSI; // RSSI valueֵ
signed char dbmRSSI; // RSSI(dbm) after calculation

//----------------- function declaration -----------------
//void SpiInit(void);
//void CpuInit(void);
void RESET_CC1100(void);
//void POWER_UP_RESET_CC1100(void);
void halSpiWriteReg(uint8_t addr, uint8_t value);
//void halSpiWriteBurstReg(uint8_t addr, uint8_t *buffer, uint8_t count);
void halSpiStrobe(uint8_t strobe);
//uint8_t halSpiReadReg(uint8_t addr);
//void halSpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t count);
//uint8_t halSpiReadStatus(uint8_t addr);
//void halRfWriteRfSettings(void);
//void halRfSendPacket(uint8_t *txBuffer, uint8_t size); 
//uint8_t halRfReceivePacket(uint8_t *rxBuffer, uint8_t *length);  
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
const RF_SETTINGS rfSettings =
{
    0x00,
    0x06,   //- FSCTRL1   Frequency synthesizer control.
    0x00,   //- FSCTRL0   Frequency synthesizer control.
    0x10,   //- FREQ2     Frequency control word, high byte.
    0xA7,   //- FREQ1     Frequency control word, middle byte.
    0x62,   //- FREQ0     Frequency control word, low byte.
    0xF6,   //- MDMCFG4   Modem configuration. [3:0]------- DRATE_E 6 
    0x83,   //- MDMCFG3   Modem configuration. [7:0]--------DRATE_M=131
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
    0x04,   // PKTCTRL1  Packet automation control. --[2]=1 enable APPEND_STATUS(RRSI+LQI) and CRC
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

//----------- transmit  data packet ---------------------
void halRfSendPacket(uint8_t *txBuffer, uint8_t size) 
{
    int i;
    uint8_t buf;

    halSpiWriteReg(CCxxx0_TXFIFO, size);
    halSpiWriteBurstReg(CCxxx0_TXFIFO, txBuffer, size);
    //halSpiStrobe(CCxxx0_SIDLE);
    halSpiStrobe(CCxxx0_STX); //enter transmit mode and send out data
    // Wait for GDO0 to be set -> sync transmitted
    //while (!GDO0);
    // Wait for GDO0 to be cleared -> end of packet 
    //while (GDO0);
    usleep(10000); 
    SPI_Read(&buf,1);
    while((buf & 0xf0)!=0)
    {
	usleep(10000);
	SPI_Read(&buf,1);
	printf("0X%02x\n",buf); 
    }
    halSpiStrobe(CCxxx0_SFTX);  //flush TXFIFO
}
//*****************************************************************************************
void setRxMode(void)
{
    halSpiStrobe(CCxxx0_SRX);           //��ȱ뱫�ӱʱձ״̬
}






//======================= MAIN =============================
int main(void)
{
	int len,i;
	int ret;
	uint8_t Txtmp,data[32];
	uint8_t TxBuf[5],RxBuf[5];

	len=2;
 	memset(data,0,sizeof(data));
	memset(TxBuf,0,sizeof(TxBuf));
	memset(RxBuf,0,sizeof(RxBuf));

	TxBuf[0]=0xac;
	TxBuf[1]=0xab;



	SPI_Open();

        Txtmp=0x3b;
        SPI_Write(&Txtmp,1); //clear RxBuf in cc1101
	Txtmp=0x3b|READ_SINGLE;
        ret=SPI_Transfer(&Txtmp,RxBuf,1,1);
	printf("RXBYTES: ret=%d, =x%02x\n",ret,RxBuf[0]);

	Txtmp=0x3b|READ_BURST;
	ret=SPI_Write_then_Read(&Txtmp,1,RxBuf,2); //RXBYTES
	printf("RXBYTES: ret=%d, =x%02x %02x\n",ret,RxBuf[0],RxBuf[1]);

        //halSpiWriteReg(0x25,0x7f);
	data[0]=data[1]=data[2]=data[3]=data[4]=0xff;
	halSpiWriteBurstReg(0x25,data,5);
	//Txtmp=0x25|READ_BURST;
	//ret=SPI_Write_then_Read(&Txtmp,1,RxBuf,5); //TXBYTES
	halSpiReadBurstReg(0x25,RxBuf,5);
	printf("halSpiReadBurstReg(0x25,RxBuf,5) : ");
	for(i=0;i<5;i++)
	    printf("%02x",RxBuf[i]);
	printf("\n");

	printf("halSpiReadReg(0x25)=x%02x\n",halSpiReadReg(0x25));

	printf("halSpiReadReg(0x31)=x%02x\n",halSpiReadReg(0x31));
	printf("halSpiReadStatus(0x31) Chip ID: x%02x\n",halSpiReadStatus(0x31));

	//-----init CC1101-----
	halRfWriteRfSettings();
	halSpiWriteBurstReg(CCxxx0_PATABLE,PaTabel,8);

        halRfSendPacket(data,32);

	SPI_Close();
}
