#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h> 
#include <string.h>
#define  RECV_LOOP_COUNT 100
int main()
{
    unsigned short expect_sn=0;
    int sockfd;
    struct sockaddr_in my_addr;
    //struct sockaddr_in their_addr;
    int addr_len;
    if((sockfd=socket(AF_INET,SOCK_DGRAM,0))==-1)
    {
        printf("error in socket");
        return -2;
    }
    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(9449);
    my_addr.sin_addr.s_addr=inet_addr("127.0.0.1");


    memset(my_addr.sin_zero,0,8);
    addr_len = sizeof(struct sockaddr);
    struct sockaddr_in send_addr;
    send_addr.sin_family=AF_INET;
    send_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    send_addr.sin_port=htons(6666);
    memset(my_addr.sin_zero,0,8);
    int sens_addr_len = sizeof(struct sockaddr_in);
    char sends[]="hello";
    char input[100];
    while(1)
    {
        scanf("%s",input);
        sendto(sockfd,sends,6,0,(struct sockaddr*)&send_addr,sens_addr_len);
    }
}