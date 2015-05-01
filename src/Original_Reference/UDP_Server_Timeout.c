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
    my_addr.sin_port=htons(9450);
    my_addr.sin_addr.s_addr=inet_addr("127.0.0.1");


    memset(my_addr.sin_zero,0,8);
    addr_len = sizeof(struct sockaddr);
    int re_flag=1;
    int re_len=sizeof(int);
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&re_flag,re_len);
    if(bind(sockfd,(const struct sockaddr *)&my_addr,addr_len)==-1)
    {
        printf("error in binding\n");
        return -3;
    }

    struct timeval tv;
    fd_set readfds;
    int i=0;
    unsigned int n=0;
    char buf[1024];
    struct sockaddr addr;
    socklen_t len;
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd,&readfds);
        tv.tv_sec=2;
        tv.tv_usec=0;
        select(sockfd+1,&readfds,NULL,NULL,&tv);
        if(FD_ISSET(sockfd,&readfds))
        {
            if((n=recvfrom(sockfd,buf,1024,0,&addr,&len))>=0)
            {    
                printf("in time ,left time %d s ,%d usec\n",tv.tv_sec,tv.tv_usec);
            }
        }
        else
            printf("timeout ,left time %d s ,%d usec\n",tv.tv_sec,tv.tv_usec);
    }
    return 0;
}
