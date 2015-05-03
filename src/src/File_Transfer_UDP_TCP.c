#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

/* TCP/UDP file transfer
   2015/05/01
   Author: An-Che Teng, ES104, NCKU */

/* Function List:
    ======================= Shared Functions ======================
    filesize(file_descripter);
    log(count, sum, total);
    ======================== TCP Functions ========================
    TCP_send_binary_data(filename, socket_fd);
    TCP_receive_binary_data(filename, socket_fd);
    ======================== UDP Functions ========================
    zero_one_converter( 0_or_1 );
    UDP_send_file_size(file_descriptor, socket_fd, sockaddr);
    UDP_send_binary_data(filename, socket_fd, sockaddr);
    UDP_receive_file_size(file_descriptor, socket_fd, sockaddr);
    UDP_receive_binary_data(filename, socket_fd, sockaddr);
*/

// Function: Give the filesize of the file which fp point to
// Usage: filesize(file_descripter);
int filesize(FILE *fp)
{
    int sz;
    /* From file head (initial=0) to file tail */
    fseek(fp, 0L, SEEK_END);

    /* Now fp is at the tail of file, so equal to file size. */
    sz = ftell(fp);

    /* Let fp back to file head!! */
    fseek(fp, 0L, SEEK_SET);

    return sz;
}


// Function: Print percentage and time for every 5%
// Usage: log(count, sum, total);
int log_display(int count, int sum, int total)
{
    /* Present time */
    time_t now;
    time(&now);

    /* Calculate if exceed n*5% percentage*/
    if(100*sum/total >= count*5)
    {
        printf("%3d%% ", count*5);
        printf("%s", ctime(&now));
        count+=1;
    }

    /* Return the next n*5% percentage*/
    return count;
}


// Function: Send binary data to socket (TCP)
// Usage: TCP_send_binary_data(filename, socket_fd);
int TCP_send_binary_data(char* filename, int sockfd)
{
    /* Open the file that we wish to transfer */
    FILE *fp = fopen(filename,"rb");
    if(fp==NULL)
    {
        printf("File opern error");
        return 1;
    }

    /* Parameter for log_display */
    int sum = 0;
    int count = 0;
    int opend_filesize = filesize(fp);

    /* Change the filesize(int) which get from filesize() to string(char[]) */
    char filesize_char[100];
    sprintf(filesize_char, "%d", opend_filesize);


    /* Send filesize to socket */
    write(sockfd, filesize_char, strlen(filesize_char));

    /* Wait for server I/O */
    usleep(500);

    /* Read data from file and send it */
    while(1)
    {
        /* First read file in chunks of 256 bytes */
        unsigned char buff[256]= {0};
        int nread = fread(buff,1,256,fp);

        /* If read was success, send data. */
        if(nread > 0)
        {
            /* Print the log-message and write msg into socket*/
            sum += nread;
            count = log_display(count, sum, opend_filesize);

            write(sockfd, buff, nread);
        }

        /* Either there was error, or we reached end of file.
         * When reached the end of file, means the transmission is over! */
        if (nread < 256)
        {
            if (feof(fp))
                printf("\nTransmission is OVER!\n");
            if (ferror(fp))
                printf("Error reading\n");
            break;
        }
    }
    return 0;
}


// Function: Receive binary data from socket (TCP)
// Usage: TCP_receive_binary_data(filename, socket_fd);
int TCP_receive_binary_data(char* filename, int sockfd)
{
    /* Create file where data will be stored */
    FILE *fp;
    fp = fopen(filename,"wb");
    if(NULL == fp)
    {
        printf("Error opening file");
        return 1;
    }

    /* Parameter for read, and empty the recvBuff */
    int bytesReceived = 0;
    char recvBuff[256];
    memset(recvBuff, 0, sizeof(recvBuff));

    /* Parameter for log_display */
    int sum = 0;
    int count = 0;

    /* Receive filesize to socket, and empty the recvBuff again */
    read(sockfd, recvBuff, 256);
    int filesize = atoi(recvBuff);
    memset(recvBuff, 0, sizeof(recvBuff));

    /* Receive data in chunks of 256 bytes */
    while((bytesReceived = read(sockfd, recvBuff, 256)) > 0)
    {
        /* Print the log-message and write msg into file*/
        sum += bytesReceived;
        count = log_display(count, sum, filesize);
        fwrite(recvBuff, 1,bytesReceived,fp);
    }

    /* If read failed */
    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }

    printf("\nTransmission is OVER!\n");

    return 0;
}


// Function: Convert 0/1 (0->1 or 1->0)
// Usage: zero_one_converter( 0_or_1 );
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


// Function: Send filesize to socket
// Usage: UDP_send_file_size(file_descriptor, socket_fd, sockaddr);
int UDP_send_file_size(FILE *fp, int sockfd, struct sockaddr_in addr_to)
{
    /* Parameter for timer */
    struct timeval tv;
    fd_set readfds;

    /* Parameter for recvfrom */
    int addr_len = sizeof(struct sockaddr_in);

    /* check_ack for filesize */
    int rcvsize_ok=0;

    /* Change filesize to char */
    char filesize_char[100];
    sprintf(filesize_char, "%d", filesize(fp));

    /* Continues send filesize until the ack(2) is received */
    while(rcvsize_ok == 0)
    {
        /* Send filesize */
        sendto(sockfd,filesize_char,strlen(filesize_char),0,(struct sockaddr*)&addr_to,sizeof(addr_to));

        /* Buffer for ack_of_filesize */
        char recvSizeOK[3]= {0};

        /* Timer setting */
        FD_ZERO(&readfds);
        FD_SET(sockfd,&readfds);
        tv.tv_sec=1;
        tv.tv_usec=0;

        /*  ========================================Timer========================================  */
        select(sockfd+1,&readfds,NULL,NULL,&tv);
        /* Received in time */
        if(FD_ISSET(sockfd,&readfds))
        {
            /* Received correct */
            if( recvfrom(sockfd,recvSizeOK,sizeof(recvSizeOK),0, (struct sockaddr *)&addr_to ,&addr_len)>=0 )
            {
                rcvsize_ok = atoi(recvSizeOK);
                /* Ack correct */
                if (rcvsize_ok==2)
                    break;
                else /* Ack wrong */
                {
                    rcvsize_ok=0;
                    break;
                }
            }
        }
        /* else: Timeout */
        /*  ========================================Timer========================================  */
    }
}


// Function: Send binary data to socket
// Usage: UDP_send_binary_data(filename, socket_fd, sockaddr);
int UDP_send_binary_data(char* filename, int sockfd, struct sockaddr_in addr_to)
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
    int addr_len = sizeof(struct sockaddr_in);

    /* Parameter for loss rate */
    int loss = 0;
    int total_packet_number = 0;

    /* Send filesize */
    UDP_send_file_size(fp, sockfd, addr_to);

    /* Parameter for log_display */
    int sum = 0;
    int count = 0;
    int opend_filesize = filesize(fp);

    /* Wait for server I/O */
    usleep(500);


    /* Read data from file and send it */
    while(1)
    {
        /* First read file in chunks of 256 bytes */
        unsigned char buff[254]= {0};

        /* Buffer for received ack */
        char recvBuff[3]= {0};

        /* Read from file(fp) and record read bytes number(nread) */
        int nread = fread(buff,1,254,fp);


        /* flag = 1 when receive correct ack */
        int flag = 0;

        /* If read was success, send data. */
        if(nread > 0)
        {

            /* Print the log-message and write msg into file*/
            sum += nread;
            count = log_display(count, sum, opend_filesize);

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
                /* Received in time */
                if(FD_ISSET(sockfd,&readfds))
                {
                    if( recvfrom(sockfd,recvBuff,sizeof(recvBuff),0, (struct sockaddr *)&addr_to ,&addr_len)>=0 )
                    {
                        received_ack = atoi(recvBuff);
                        total_packet_number++;
                        /* Ack correct! */
                        if (received_ack == check_ack)
                        {
                            check_ack = zero_one_converter(check_ack);
                            flag = 1;
                        }
                    }
                    else
                    {
                        /* Receive fail */
                        total_packet_number++;
                        loss++;
                    }
                }
                else /* Timeout */
                {
                    total_packet_number++;
                    loss++;
                }
                /*  ========================================Timer========================================  */
            }
        }

        /* Either there was error, or we reached end of file.
         * When reached the end of file, means the transmission is over! */
        if (nread < 254)
        {
            if (feof(fp))
                printf("\nTransmission is OVER!\n");
            if (ferror(fp))
                printf("Error reading\n");
            break;
        }

    }

    printf("\nPacket Loss Rate: %d %%\n\n", 100*loss/total_packet_number);
    return 0;
}


// Function: Receive filesize from socket
// Usage: UDP_receive_file_size(file_descriptor, socket_fd, sockaddr);
int UDP_receive_file_size(int sockfd, struct sockaddr_in addr)
{
    /* Parameter for filesize_buff, and empty the filesize_buff */
    char filesize_buff[257];
    memset(filesize_buff, 0, sizeof(filesize_buff));
    int addr_len = sizeof(struct sockaddr_in);

    /* Receive filesize from socket */
    recvfrom(sockfd,filesize_buff,sizeof(filesize_buff),0, (struct sockaddr *)&addr ,&addr_len);

    /* Change filesize to integer */
    int filesize = atoi(filesize_buff);

    /* Send filesize_ack back */
    sendto(sockfd,"2",1,0,(struct sockaddr*)&addr,sizeof(addr));

    return filesize;
}


// Function: Receive binary data from socket
// Usage: UDP_receive_binary_data(filename, socket_fd, sockaddr);
int UDP_receive_binary_data(char* filename, int sockfd, struct sockaddr_in addr)
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
    int filesize = UDP_receive_file_size(sockfd, addr);

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

        /* Received expected packet */
        if (ack == seq_int)
        {
            /* Send ack back */
            sendto(sockfd,seq,1,0,(struct sockaddr*)&addr,sizeof(addr));

            /* Convert ack to next expected seq_number */
            ack = zero_one_converter(ack);

            /* Print the log-message and write msg into file*/
            sum += bytesReceived;
            count = log_display(count, sum, filesize);
            fwrite(msg,1,strlen(msg),fp);
        }
        else if (zero_one_converter(ack) == seq_int) /* Received previous packet */
            sendto(sockfd,re_ack,1,0,(struct sockaddr*)&addr,sizeof(addr));
        else /* Received filesize packet */
            sendto(sockfd,"2",1,0,(struct sockaddr*)&addr,sizeof(addr));
    }

    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }

    printf("\nTransmission is OVER!\n");

    return 0;
}



int main(int argc, char *argv[])
{
    /* Parameter from cmd input */
    int portnum = atoi(argv[3]);
    char *IP_address = argv[4];
    struct in_addr temp;
    struct hostent *server;

    /* Solve the IP-address problem (localhost or direct IP address) */
    if (strncmp(argv[4],"localhost",strlen("localhost"))==0)
    {
        /* localhost */
        server = gethostbyname(IP_address);
        bcopy((char *)server->h_addr, (char *)&temp.s_addr, server->h_length);
        IP_address = inet_ntoa(temp);
    } else {
        /* Direct IP address */
        IP_address = argv[4];
    }

    /* Wrong usage when argc is not enough */
    if (argc < 5) {
        fprintf(stderr,"usage %s  TCP/UDP  SEND/RECV  Port  Hostname/IP-address  filename\n", argv[0]);
        exit(0);
    }

    /* Decide which mode is used.
     * Totally there are four mode:
     * 	  1. TCP and SEND file (client)
     *    2. TCP and RECV file (server)
     *	  3. UDP and SEND file (client)
     *    4. UDP and RECV file (server)
     * Here decide the mode according to cmd input. */

    if (strncmp(argv[1],"TCP",strlen("TCP"))==0)
    {
        printf("\n\nTCP mode!\n\n");
        if(strncmp(argv[2],"SEND",strlen("SEND"))==0)
        {
            int sockfd = 0;
            struct sockaddr_in serv_addr;

            /* Create a socket first */
            if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
            {
                printf("\n Error : Could not create socket \n");
                return 1;
            }

            /* Initialize sockaddr_in data structure */
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(portnum); // port
            serv_addr.sin_addr.s_addr = inet_addr(IP_address);

            /* Attempt a connection */
            if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
            {
                printf("\n Error : Connect Failed \n");
                return 1;
            }

            printf("\nConnect success\n\n");

            /* TCP send */
            TCP_send_binary_data(strtok(argv[5], " "),sockfd);

            return 0;

        }
        else if(strncmp(argv[2],"RECV",strlen("RECV"))==0)
        {
            int listenfd = 0;
            int connfd = 0;
            struct sockaddr_in serv_addr;
            char sendBuff[1025];

            /* Create a socket first */
            listenfd = socket(AF_INET, SOCK_STREAM, 0);

            printf("\nSocket retrieve success\n\n");

            memset(&serv_addr, '0', sizeof(serv_addr));
            memset(sendBuff, '0', sizeof(sendBuff));

            /* Initialize sockaddr_in data structure */
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            serv_addr.sin_port = htons(portnum);

            /* Bind the address to socket */
            bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));

            /* Listen to client */
            if(listen(listenfd, 10) == -1)
            {
                printf("Failed to listen\n");
                return -1;
            }

            /* Accept client connect */
            connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL);

            /* TCP receive */
            TCP_receive_binary_data("output_data.txt", connfd);

            close(connfd);
            return 0;

        }
    }
    else if (strncmp(argv[1],"UDP",strlen("UDP"))==0)
    {
        printf("\n\nUDP mode!\n\n");
        if(strncmp(argv[2],"SEND",strlen("SEND"))==0)
        {
            int fd,r;
            struct sockaddr_in addr_to;
            struct sockaddr_in addr_from;

            /* Create a socket first */
            if((fd=socket(AF_INET,SOCK_DGRAM,0))<0)
            {
                perror("socket create error!\n");
                exit(-1);
            }

            /* Server IP Address setting*/
            addr_to.sin_family=AF_INET;
            addr_to.sin_port=htons(portnum);
            addr_to.sin_addr.s_addr=inet_addr(IP_address);

            /* Client IP Address setting*/
            addr_from.sin_family=AF_INET;
            addr_from.sin_port=htons(0); /* Get a free port */
            addr_from.sin_addr.s_addr=htonl(INADDR_ANY); /* Get client-self IP address */

            /* Bind the address to socket */
            if(bind(fd,(struct sockaddr*)&addr_from,sizeof(addr_from))<0)
            {
                printf("Bind error!\n");
                close(fd);
                exit(-1);
            }

            printf("\nClient Bind successfully.\n\n");

            /* UDP send */
            UDP_send_binary_data(strtok(argv[5], " "),fd,addr_to);

            close(fd);
            return 0;
        }
        else if(strncmp(argv[2],"RECV",strlen("RECV"))==0)
        {
            int listenfd,fd;
            struct sockaddr_in addr;
            struct sockaddr_in from;

            /* Create a socket first */
            if((fd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
            {
                perror("socket create error!\n");
                exit(-1);
            }

            /* Server IP Address setting*/
            addr.sin_family=AF_INET;
            addr.sin_port=htons(portnum);
            addr.sin_addr.s_addr=htonl(INADDR_ANY);

            /* Bind the address to socket */
            if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))<0)
            {
                printf("Bind error!\n");
                close(fd);
                exit(-1);
            }

            printf("\nServer bind successfully.\n\n");

            /* UDP receive */
            UDP_receive_binary_data("output_data.txt", fd, from);

            close(fd);
            return 0;

        }
    }
}