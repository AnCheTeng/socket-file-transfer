//server.c(UDP)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>//for sockaddr_in
#include <arpa/inet.h>//for socket
#include <time.h>
#include <string.h>
#include <sys/socket.h>

int zero_one_converter(int number)
{
    if (number==0)
        return 1;
    else if (number==1)
        return 0;
    else
    {
        printf("Wrong input!\n");
        return 2;
    }
}

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

int filesize(FILE *fp)
{
    int sz;
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return sz;
}

int receive_file_size(int sockfd, struct sockaddr_in addr)
{
    char recvBuff[257];
    memset(recvBuff, 0, sizeof(recvBuff));
    int addr_len = sizeof(struct sockaddr_in);

    recvfrom(sockfd,recvBuff,sizeof(recvBuff),0, (struct sockaddr *)&addr ,&addr_len);
    int filesize = atoi(recvBuff);
    sendto(sockfd,"2",1,0,(struct sockaddr*)&addr,sizeof(addr));    

    return filesize;
}
// Receive binary data from socket
// Usage: receive_binary_data(filename, socket_fd, filesize, sockaddr);
int receive_binary_data(char* filename, int sockfd, struct sockaddr_in addr)
{
    /* Create file where data will be stored */
    FILE *fp;
    fp = fopen(filename,"wb");
    if(NULL == fp)
    {
        printf("Error opening file");
        return 1;
    }

    /* Parameter for recvfrom */
    int bytesReceived = 256;
    char recvBuff[257];
    int addr_len = sizeof(struct sockaddr_in);

    /* For acknowledge check */
    int ack = 0;

    /* Parameter for log_display */
    int sum = 0;
    int count = 0;

    /* Get filesize from sender */
    int filesize = receive_file_size(sockfd, addr);

    /* Receive data in chunks of 256 bytes */
    while( bytesReceived > 255 )
    {
        memset(recvBuff, 0, sizeof(recvBuff));
        bytesReceived = recvfrom(sockfd,recvBuff,sizeof(recvBuff),0, (struct sockaddr *)&addr ,&addr_len);
        
        /* Divide the package */
        char *seq = strtok(recvBuff, " ");
        char *msg = strtok(NULL, "");
        int seq_int = atoi(seq);

        /* Generate re_ack for duplicate packet */
        char *re_ack = "0";
        if(ack == 1)
            re_ack += 1; /* Convert '0' to '1' */

        if (ack == seq_int)
        {
            sendto(sockfd,seq,1,0,(struct sockaddr*)&addr,sizeof(addr));
            ack = zero_one_converter(ack);
            sum += bytesReceived;
            count = log_display(count, sum, filesize);
            fwrite(msg,1,strlen(msg),fp);
        }
        else if (zero_one_converter(ack) == seq_int)
            sendto(sockfd,re_ack,1,0,(struct sockaddr*)&addr,sizeof(addr));
        else
            sendto(sockfd,"2",1,0,(struct sockaddr*)&addr,sizeof(addr));
    }

    count+=1;
    log_display(count, sum, filesize);

    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }

    printf("Transmission is OVER!\n");

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

    addr.sin_family=AF_INET;
    addr.sin_port=htons(6666);
    addr.sin_addr.s_addr=inet_addr("127.0.0.1");

    if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))<0)
    {
        printf("Bind error!\n");
        close(fd);
        exit(-1);
    }
    printf("Server bind successfully.\n");

    struct sockaddr_in from;

    receive_binary_data("output_data.txt", fd, from);

    close(fd);
    return 0;
}