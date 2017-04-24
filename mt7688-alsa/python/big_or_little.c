#include <stdio.h>  
//判断操作系统是大端还是小端  
bool IsBig_Edian()  
{  
    union my  
    {  
        int a;  
        char b;
int c;
    };  
   union my t;  
    t.a=0x12345678;//union用法：此时
    if(t.c==0x78){
    return 1;
 }  
}  
int main()  
{  
    printf("%d\n",IsBig_Edian());  
    return 0;  
}  