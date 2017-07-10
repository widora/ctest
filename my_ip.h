/*--------------------------------------------
           Return char* of IP address
------------------------------------------*/

#ifndef __MY_IP_H__
#define __MY_IP_H__

/* With reference to StackOverflow: http://stackoverflow.com/questions/212528/Linux-c-get-the-ip-address-of-local-computer */
#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

char* getMyIP(void)
{
  static char str_myip[INET_ADDRSTRLEN]={0};
  struct ifaddrs *ifaddr;
  struct ifaddrs *ifAddrStruct=NULL;
  void *tmpAddrPtr=NULL;

   getifaddrs(&ifaddr);
   ifAddrStruct=ifaddr;
/*   -1 return SUCCESS ?????????
  if(getifaddrs(&ifAddrStruct) == -1);//---!!!MUST free it later
  {
	perror("getifaddrs");
	strcpy(str_myip,"Cont WiFi Fails!");
	return str_myip;
   }
*/
  while(ifAddrStruct!=NULL)
  {
  	if(ifAddrStruct->ifa_addr->sa_family==AF_INET)  //check if it's valid IP4 address
	{
		tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
		//char addressBuffer[INET_ADDRSTRLEN];
		inet_ntop(AF_INET,tmpAddrPtr,str_myip,INET_ADDRSTRLEN);// convert to Dotted Decimal Notation
		if(!strcmp(ifAddrStruct->ifa_name,"wlan0")) //--return wlan0 IP
		{
			printf("%s IP Address %s\n", ifAddrStruct->ifa_name,str_myip);
			freeifaddrs(ifaddr);
	 		return str_myip;
		}
 	}
	else if(ifAddrStruct->ifa_addr->sa_family==AF_INET6) //check if it's valid IP6 address
	{
		tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
		char addressBuffer[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6,tmpAddrPtr,addressBuffer,INET6_ADDRSTRLEN);
		printf("%s IPv6 Address %s\n", ifAddrStruct->ifa_name,addressBuffer);
	}
	usleep(100000);
	ifAddrStruct=ifAddrStruct->ifa_next;
  }
  
  strcpy(str_myip,"Cont WiFi Fails!");
  if(ifaddr != NULL)
	  freeifaddrs(ifaddr);  //!!!! free ifaddr
  return str_myip;

}


#endif
