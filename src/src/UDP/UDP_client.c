//client.c(UDP)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>//for sockaddr_in
#include <arpa/inet.h>//for socket
#include <time.h>
#include <string.h>

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

// Send binary data to socket
// Usage: send_binary_data(filename, socket_fd, sockaddr);
int send_binary_data(char* filename, int sockfd, struct sockaddr_in addr_to)
{
    /* Open the file that we wish to transfer */
    FILE *fp = fopen(filename,"rb");
    if(fp==NULL)
    {
        printf("File opern error");
        return 1;
    }

    int check_ack = 0;
    struct timeval tv;
    fd_set readfds;
    int loss = 0;
    /* Read data from file and send it */
    while(1)
    {
        /* First read file in chunks of 256 bytes */
        unsigned char buff[254]= {0};
        char recvBuff[3]= {0};
        int nread = fread(buff,1,254,fp);
        int addr_len = sizeof(struct sockaddr_in);

        unsigned int n=0;

        /* If read was success, send data. */
        if(nread > 0)
        {
            if(check_ack == 1)
            {
                char packet[256] = "1 ";
                strcat(packet,buff);
                while(check_ack==1)
                {
                    sendto(sockfd,packet,strlen(packet),0,(struct sockaddr*)&addr_to,sizeof(addr_to));
                    FD_ZERO(&readfds);
                    FD_SET(sockfd,&readfds);
                    tv.tv_sec=2;
                    tv.tv_usec=0;
                    select(sockfd+1,&readfds,NULL,NULL,&tv);
                    if(FD_ISSET(sockfd,&readfds))
                    {
                        if((n=recvfrom(sockfd,recvBuff,sizeof(recvBuff),0, (struct sockaddr *)&addr_to ,&addr_len))>=0)
                        {
                            // printf("in time ,left time %d s ,%d usec\n",tv.tv_sec,tv.tv_usec);
                            int cack = atoi(recvBuff);
                            if (cack==check_ack)
                            {
                                check_ack = 0;
                            }
                        }
                        else
                            loss++;
                    }
                    else
                        loss++;//printf("timeout\n");

                }

            }
            else if (check_ack == 0)
            {
                char packet[256] = "0 ";
                strcat(packet,buff);

                while(check_ack==0)
                {

                    sendto(sockfd,packet,strlen(packet),0,(struct sockaddr*)&addr_to,sizeof(addr_to));

                    FD_ZERO(&readfds);
                    FD_SET(sockfd,&readfds);
                    tv.tv_sec=2;
                    tv.tv_usec=0;
                    select(sockfd+1,&readfds,NULL,NULL,&tv);
                    if(FD_ISSET(sockfd,&readfds))
                    {
                        if((n=recvfrom(sockfd,recvBuff,sizeof(recvBuff),0, (struct sockaddr *)&addr_to ,&addr_len))>=0)
                        {
                            // printf("in time ,left time %d s ,%d usec\n",tv.tv_sec,tv.tv_usec);
                            int cack = atoi(recvBuff);
                            if (cack==check_ack)
                            {
                                check_ack = 1;
                            }
                        }
                        else
                            loss++;
                    }
                    else
                        loss++;//printf("timeout\n");
                }
            }
            //printf("Sending. \n");
            //write(sockfd, buff, nread);
        }

        /* There is something tricky going on with read ..
         * Either there was error, or we reached end of file. */
        if (nread < 254)
        {
            if (feof(fp))
                printf("End of file. Transmission is over.\n");
            // sendto(sockfd,"2",1,0,(struct sockaddr*)&addr_to,sizeof(addr_to));
            if (ferror(fp))
                printf("Error reading\n");
            break;
        }

    }

    printf("Loss packet: %d\n", loss);

    return 0;
}


int main()
{
    int fd,r;
    struct sockaddr_in addr_to;
    struct sockaddr_in addr_from;

    if((fd=socket(AF_INET,SOCK_DGRAM,0))<0)
    {
        perror("socket create error!\n");
        exit(-1);
    }
    printf("socket fd=%d\n",fd);

    //目標服務器地址
    addr_to.sin_family=AF_INET;
    addr_to.sin_port=htons(6666);
    addr_to.sin_addr.s_addr=inet_addr("127.0.0.1");

    //本機地址
    addr_from.sin_family=AF_INET;
    addr_from.sin_port=htons(0);//獲得任意空閑端口
    addr_from.sin_addr.s_addr=htons(INADDR_ANY);//獲得本機地址

    if(bind(fd,(struct sockaddr*)&addr_from,sizeof(addr_from))<0)
    {
        printf("Bind error!\n");
        close(fd);
        exit(-1);
    }

    printf("Client Bind successfully.\n");

    send_binary_data("test_input100mb.txt",fd,addr_to);
    // char buf[255];
    // int len;
    // int hi=0;
    // while(hi==0)
    // {
    //     r=read(0,buf,sizeof(buf));
    //     if(r<0)
    //     {
    //         break;
    //     }
    //     len=sendto(fd,buf,r,0,(struct sockaddr*)&addr_to,sizeof(addr_to));
    //     if(len==-1)
    //     {
    //         printf("send falure!\n");
    //     }
    //     else
    //     {
    //         printf("%d bytes have been sended successfully!\n",len);
    //     }
    //     if(string_compare(buf,"OVER")==0)
    //     {
    //         hi = 1;
    //     }
    // }
    close(fd);
    return 0;
}