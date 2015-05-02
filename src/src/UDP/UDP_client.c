//client.c(UDP)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>//for sockaddr_in
#include <arpa/inet.h>//for socket
#include <time.h>
#include <string.h>

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

    /* Parameter for acknowledge check */
    int check_ack = 0;
    int received_ack;

    /* Parameter for timer */
    struct timeval tv;
    fd_set readfds;

    /* Parameter for loss rate */
    int loss = 0;
    int packet_number = 0;

    /* Read data from file and send it */
    while(1)
    {
        /* First read file in chunks of 256 bytes */
        unsigned char buff[254]= {0};

        char recvBuff[3]= {0};
        int nread = fread(buff,1,254,fp);
        int addr_len = sizeof(struct sockaddr_in);

        /* flag = 1 when receive correct ack */
        int flag = 0;

        /* If read was success, send data. */
        if(nread > 0)
        {
            while(flag == 0)
            {
                /* Empty packet and set the header */
                char packet[256] = {0};
                packet[1] = ' ';
                packet[0] = '0';
                if(check_ack == 1)
                    packet[0] = '1';

                /* Combine the header and message */
                strcat(packet,buff);

                /* Send packet into socket */
                sendto(sockfd,packet,strlen(packet),0,(struct sockaddr*)&addr_to,sizeof(addr_to));

                /* Timer setting */
                FD_ZERO(&readfds);
                FD_SET(sockfd,&readfds);
                tv.tv_sec=1;
                tv.tv_usec=0;

                /*  ========================================Timer========================================  */
                select(sockfd+1,&readfds,NULL,NULL,&tv);
                if(FD_ISSET(sockfd,&readfds))
                {
                    if( recvfrom(sockfd,recvBuff,sizeof(recvBuff),0, (struct sockaddr *)&addr_to ,&addr_len)>=0 )
                    {
                        received_ack = atoi(recvBuff);
                        if (received_ack == check_ack)
                        {
                            check_ack = zero_one_converter(check_ack);
                            flag = 1;
                        }
                    }
                    else
                        loss++;
                }
                else
                    loss++; /* Time out */
                /*  ========================================Timer========================================  */
            }
        }

        /* There is something tricky going on with read ..
         * Either there was error, or we reached end of file. */
        if (nread < 254)
        {
            if (feof(fp))
                printf("End of file. \nTransmission is OVER!\n");
            if (ferror(fp))
                printf("Error reading\n");
            break;
        }

    }

    printf("Lossed packet: %d\n", loss);
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

    /* Server IP Address setting*/
    addr_to.sin_family=AF_INET;
    addr_to.sin_port=htons(6666);
    addr_to.sin_addr.s_addr=inet_addr("127.0.0.1");

    /* Client IP Address setting*/
    addr_from.sin_family=AF_INET;
    addr_from.sin_port=htons(0); /* Get a free port */
    addr_from.sin_addr.s_addr=htons(INADDR_ANY); /* Get client-self IP address */

    if(bind(fd,(struct sockaddr*)&addr_from,sizeof(addr_from))<0)
    {
        printf("Bind error!\n");
        close(fd);
        exit(-1);
    }

    printf("Client Bind successfully.\n");

    send_binary_data("sample_file1.txt",fd,addr_to);

    close(fd);
    return 0;
}