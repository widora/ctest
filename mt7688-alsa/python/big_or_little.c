#include <stdio.h>  
//�жϲ���ϵͳ�Ǵ�˻���С��  
bool IsBig_Edian()  
{  
    union my  
    {  
        int a;  
        char b;
int c;
    };  
   union my t;  
    t.a=0x12345678;//union�÷�����ʱ
    if(t.c==0x78){
    return 1;
 }  
}  
int main()  
{  
    printf("%d\n",IsBig_Edian());  
    return 0;  
}  