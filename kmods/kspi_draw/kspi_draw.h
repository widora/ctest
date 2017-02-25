
#ifndef __SPI_DRAW_H__
#define __SPI_DRAW_H__

//#include <asm/mach-ralink/rt_mmap.h>

/*    ------------------------------------spi----------------------------------------------------      */
 #define RALINK_SPI_BASE 0xB0000B00 //0x10000B00+0xA000000

 #define SPI_REG_CTL             (RALINK_SPI_BASE + 0x00)
 #define SPI_REG_OPCODE          (RALINK_SPI_BASE + 0x04)
 #define SPI_REG_DATA0           (RALINK_SPI_BASE + 0x08)
 #define SPI_REG_DATA(x)         (SPI_REG_DATA0 + (x * 4))
 #define SPI_REG_MASTER          (RALINK_SPI_BASE + 0x28)
 #define SPI_REG_MOREBUF         (RALINK_SPI_BASE + 0x2c)
 #define SPI_REG_Q_CTL           (RALINK_SPI_BASE + 0x30)
 #define SPI_REG_CS_POLAR        (RALINK_SPI_BASE + 0x38)
 #define SPI_REG_SPACE_CR        (RALINK_SPI_BASE + 0x3c)

 #define SPI_CTL_START           0x00000100
 #define SPI_CTL_BUSY            0x00010000
 #define SPI_CTL_TXCNT_MASK      0x0000000f
 #define SPI_CTL_RXCNT_MASK      0x000000f0
 #define SPI_CTL_TX_RX_CNT_MASK  0x000000ff
 #define SPI_CTL_SIZE_MASK       0x00180000
 #define SPI_CTL_ADDREXT_MASK    0xff000000

 #define SPI_MBCTL_TXCNT_MASK            0x000001ff
 #define SPI_MBCTL_RXCNT_MASK            0x001ff000
 #define SPI_MBCTL_TX_RX_CNT_MASK        (SPI_MBCTL_TXCNT_MASK | SPI_MBCTL_RXCNT_MASK)
 #define SPI_MBCTL_CMD_MASK              0x3f000000

 #define SPI_CTL_CLK_SEL_MASK    0x03000000
 #define SPI_OPCODE_MASK         0x000000ff

 #define SPI_STATUS_WIP          STM_STATUS_WIP

 #define SPI_REG_CPHA            BIT(5)
 #define SPI_REG_CPOL            BIT(4)
 #define SPI_REG_LSB_FIRST       BIT(3)
 #define SPI_REG_MODE_BITS       (SPI_CPOL | SPI_CPHA | SPI_LSB_FIRST | SPI_CS_HIGH)

 #define SPI_CPHA                0x01
 #define SPI_CPOL                0x02

 #define SPI_MODE_0              (0|0)
 #define SPI_MODE_1              (0|SPI_CPHA)
 #define SPI_MODE_2              (SPI_CPOL|0)
 #define SPI_MODE_3              (SPI_CPOL|SPI_CPHA)

 #define SPI_CS_HIGH             0x04
 #define SPI_LSB_FIRST           0x08
 #define SPI_3WIRE               0x10
 #define SPI_LOOP                0x20
 #define SPI_NO_CS               0x40
 #define SPI_READY               0x80

 struct base_spi {
         struct clk              *clk;
         spinlock_t              lock;
         unsigned int    speed;
         unsigned int    sys_freq;
         unsigned char           *start;
         u32                     max_speed_hz;
         u8                      chip_select;
         u8                      mode;
         u8                      tx_buf[36];  // 32x8 data registers
         u8                      tx_len;
         u8                      rx_buf[36];
         u8                      rx_len;
 };

 /* ------------------------------GPIO---------------------------------------------- */
//#include <asm/rt2880/rt_mmap.h>
 #define RALINK_SYSCTL_BASE 0xB0000600 // 0xA0000000+x10000600  direct reflect
 #define RALINK_PRGIO_ADDR 0xB0000600

 #define RALINK_SYSCTL_ADDR              RALINK_SYSCTL_BASE      // system control
 #define RALINK_REG_GPIOMODE             (RALINK_SYSCTL_ADDR + 0x60)

 #define RALINK_IRQ_ADDR                 RALINK_INTCL_BASE
// #define RALINK_PRGIO_ADDR               RALINK_PIO_BASE // Programmable I/O

 #define RALINK_REG_INTENA               (RALINK_IRQ_ADDR   + 0x80)
 #define RALINK_REG_INTDIS               (RALINK_IRQ_ADDR   + 0x78)

 #define RALINK_REG_PIOINT               (RALINK_PRGIO_ADDR + 0x90)
 #define RALINK_REG_PIOEDGE              (RALINK_PRGIO_ADDR + 0xA0)
 #define RALINK_REG_PIORENA              (RALINK_PRGIO_ADDR + 0x50)
 #define RALINK_REG_PIOFENA              (RALINK_PRGIO_ADDR + 0x60)
 #define RALINK_REG_PIODATA              (RALINK_PRGIO_ADDR + 0x20)
 #define RALINK_REG_PIODIR               (RALINK_PRGIO_ADDR + 0x00)
 #define RALINK_REG_PIOSET               (RALINK_PRGIO_ADDR + 0x30)
 #define RALINK_REG_PIORESET             (RALINK_PRGIO_ADDR + 0x40)


#endif
