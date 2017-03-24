#include <stdio.h>
#include <stdlib.h> //exit()


/*---------------------- IIR_FILTER -----------------------------------
4 order IIR filter Fl=300Hz,Fh=3400Hz,Fs=8kHz
p_in_data:   pointer to start of input data
p_out_data:  pointer to start of output data
count:       count of data number in unit of int16_t
p_ind_data and p_out_data maybe the same!!
----------------------------------------------------------------------*/
static int IIR_filter(int16_t *p_in_data, int16_t *p_out_data, int count)
{
int ret=0;
  //----factors for 4 order IIR filter Fl=300Hz,Fh=3400Hz,Fs=8kHz
static  double IIR_B[5]={ 0.6031972438993, 0, -1.206394487799, 0, 0.6031972438993 };
static  double IIR_A[5]={ 1,-0.325257157029, -1.004332872001, 0.1022259821442, 0.3705866844043 };
static  double w[5]={0.0, 0.0, 0.0, 0.0, 0.0};
int k;

for(k=0;k<count;k++)
 {
	w[0]=(*p_in_data)-IIR_A[1]*w[1]-IIR_A[2]*w[2]-IIR_A[3]*w[3]-IIR_A[4]*w[4];
	(*p_out_data)=IIR_B[0]*w[0]+IIR_B[1]*w[1]+IIR_B[2]*w[2]+IIR_B[3]*w[3]+IIR_B[4]*w[4];

	w[4]=w[3];
	w[3]=w[2];
	w[2]=w[1];
	w[1]=w[0];

	p_in_data++;
	p_out_data++;
 }

return ret;
}


int main(int argc,char* argv[])
{
FILE *fin,*fout;
int16_t data_in[32]; //data for readin buffer
int16_t data_out[32];
int count=0;

printf("hello file!\n");


fin=fopen("/tmp/record.raw","r");
if(fin == NULL){
	printf("fail to open input file!\n");
	exit(-1);
 }
fout=fopen("/tmp/record_new.raw","w");
if(fout == NULL){
	printf("fail to open output file!\n");
	exit(-1);
 }

do{
	count=fread(data_in,sizeof(data_in),1,fin); //count = 1, chunks of data, not bytes of data!
	//printf("Read data count=%d\n",count);
	IIR_filter(data_in,data_in,32); //apply IIR_filter

	if(count>0)
		count=fwrite(data_in,sizeof(data_in),1,fout);
	//printf("Write data count=%d\n",count);
}while(count > 0);

close(fin);
close(fout);
return 1;

}
