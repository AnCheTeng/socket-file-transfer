//server.c(UDP)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>//for sockaddr_in
#include <arpa/inet.h>//for socket
#include <time.h>
#include <string.h>
#include <sys/socket.h>

int log_display(int count, int sum, int total)
{
    time_t now;
    time(&now);

    if(100*sum/total >= count*5)
    {
        printf("%3d%% ", count*5);
        printf("%s", ctime(&now));
        count+=1;
    }
    return count;
}

int string_compare(char* message, char* checkmsg)
{
    if (strncmp(message,checkmsg,strlen(checkmsg))==0)
        return 0;
    else
        return 1;
}

// Receive binary data from socket
// Usage: receive_binary_data(filename, socket_fd, filesize, sockaddr);
int receive_binary_data(char* filename, int sockfd, int filesize, struct sockaddr_in addr)
{
    /* Create file where data will be stored */
    FILE *fp;
    int bytesReceived = 256;
    char recvBuff[257];
    int addr_len = sizeof(struct sockaddr_in);
    memset(recvBuff, '0', sizeof(recvBuff));
    int ack = 0;
    int sum = 0;
    int count = 0;

    fp = fopen(filename,"wb");
    if(NULL == fp)
    {
        printf("Error opening file");
        return 1;
    }

    /* Receive data in chunks of 256 bytes */
    while( bytesReceived > 255 )
    {
        memset(recvBuff, 0, sizeof(recvBuff));
        bytesReceived = recvfrom(sockfd,recvBuff,sizeof(recvBuff),0, (struct sockaddr *)&addr ,&addr_len);
        char *pack = strtok(recvBuff, " ");
        char *msg = strtok(NULL, "");
        // printf("%s\n",msg);
        int apack = atoi(pack);
        if (ack == 0)
        {
            if(ack==apack)
            {
                sendto(sockfd,"0",1,0,(struct sockaddr*)&addr,sizeof(addr));
                ack = 1;
                sum += bytesReceived;
                count = log_display(count, sum, filesize);
                fwrite(msg,1,strlen(msg),fp);
            }
            else
                sendto(sockfd,"1",1,0,(struct sockaddr*)&addr,sizeof(addr));
        }
        else if (ack==1)
        {
            if(ack==apack)
            {
                sendto(sockfd,"1",1,0,(struct sockaddr*)&addr,sizeof(addr));
                ack = 0;
                sum += bytesReceived;
                count = log_display(count, sum, filesize);
                fwrite(msg,1,strlen(msg),fp);
            }
            else
                sendto(sockfd,"0",1,0,(struct sockaddr*)&addr,sizeof(addr));
        }
        /* Print the log-message */
        // sum += bytesReceived;
        // count = log_display(count, sum, filesize);
        // fwrite(recvBuff,1,bytesReceived,fp);
    }

    count+=1;
    log_display(count, sum, filesize);

    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }

    printf("OVER!\n");

    return 0;
}


int main()
{

    struct sockaddr_in addr;
    int listenfd,fd;

    if((fd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
        perror("socket create error!\n");
        exit(-1);
    }

    printf("socket fd=%d\n",fd);


    addr.sin_family=AF_INET;
    addr.sin_port=htons(6666);
    addr.sin_addr.s_addr=inet_addr("127.0.0.1");

    if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))<0)
    {
        printf("Bind error!\n");
        close(fd);
        exit(-1);
    }
    printf("Bind successfully.\n");

    struct sockaddr_in from;

    receive_binary_data("output_data.txt", fd, 13999, from);

    // char buf[255];

    // socklen_t len;
    // len=sizeof(from);
    // int hi=0;
    // while(hi==0)
    // {
    //     listenfd=recvfrom(fd,buf,sizeof(buf),0,(struct sockaddr*)&from,&len);
    //     if(listenfd>0)
    //     {
    //         buf[listenfd]=0;
    //         printf("The message received for %s is :%s\n",inet_ntoa(from.sin_addr),buf);
    //     }
    //     else
    //     {
    //         break;
    //     }
    //     if(string_compare(buf,"OVER")==0)
    //     {
    //         hi = 1;
    //     }
    // }

    close(fd);
    return 0;
}