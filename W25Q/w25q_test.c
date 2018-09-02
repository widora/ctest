#include "spi.h"

int main()
{
	//const uint8_t TxBuf[]=

	//----- init and open spi dev -----
        if( SPI_Open() != 0 )
                return -1;

	printf("spi df opened successfuly!\n");

        //int SPI_Write_then_Read(const uint8_t *TxBuf, int n_tx, uint8_t *RxBuf, int n_rx)




	SPI_Close();

}
