#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

int filesize(FILE *fp)
{
    int sz;
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return sz;
}


// Print percentage and time for every 5%
// Usage: log(count, sum, total);
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

// Send binary data to socket
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

    char filesize_char[100];
    sprintf(filesize_char, "%d", filesize(fp));
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
            //printf("Sending. \n");
            write(sockfd, buff, nread);
        }

        /* There is something tricky going on with read ..
         * Either there was error, or we reached end of file. */
        if (nread < 256)
        {
            if (feof(fp))
                printf("Transmission is OVER!\n");
            if (ferror(fp))
                printf("Error reading\n");
            break;
        }
    }
    return 0;
}



// Receive binary data from socket
// Usage: TCP_receive_binary_data(filename, socket_fd);
int TCP_receive_binary_data(char* filename, int sockfd)
{
    /* Create file where data will be stored */
    FILE *fp;
    int bytesReceived = 0;
    char recvBuff[256];
    memset(recvBuff, 0, sizeof(recvBuff));

    int sum = 0;
    int count = 0;

    fp = fopen(filename,"wb");
    if(NULL == fp)
    {
        printf("Error opening file");
        return 1;
    }

    read(sockfd, recvBuff, 256);
    int filesize = atoi(recvBuff);
    memset(recvBuff, 0, sizeof(recvBuff));

    /* Receive data in chunks of 256 bytes */
    while((bytesReceived = read(sockfd, recvBuff, 256)) > 0)
    {
        /* Print the log-message */
        sum += bytesReceived;
        count = log_display(count, sum, filesize);
        fwrite(recvBuff, 1,bytesReceived,fp);
    }

    count+=1;
    log_display(count, sum, filesize);

    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }
    return 0;
}


int UDP_send_file_size(FILE *fp, int sockfd, struct sockaddr_in addr_to)
{
    struct timeval tv;
    fd_set readfds;
    int addr_len = sizeof(struct sockaddr_in);

    int rcvsize_ok=0;

    /* Send filesize */
    char filesize_char[100];
    sprintf(filesize_char, "%d", filesize(fp));
    
    while(rcvsize_ok == 0)
    {
        sendto(sockfd,filesize_char,strlen(filesize_char),0,(struct sockaddr*)&addr_to,sizeof(addr_to));
        /* Timer setting */
        
        char recvSizeOK[3]= {0};

        FD_ZERO(&readfds);
        FD_SET(sockfd,&readfds);
        tv.tv_sec=1;
        tv.tv_usec=0;

        /*  ========================================Timer========================================  */
        select(sockfd+1,&readfds,NULL,NULL,&tv);
        if(FD_ISSET(sockfd,&readfds))
        {
            if( recvfrom(sockfd,recvSizeOK,sizeof(recvSizeOK),0, (struct sockaddr *)&addr_to ,&addr_len)>=0 )
            {
                rcvsize_ok = atoi(recvSizeOK);
                break;
            }
        }
    }    
}

// Send binary data to socket
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

    /* Wait for server I/O */
    usleep(500);


    /* Read data from file and send it */
    while(1)
    {
        /* First read file in chunks of 256 bytes */
        unsigned char buff[254]= {0};

        char recvBuff[3]= {0};
        int nread = fread(buff,1,254,fp);
        

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
                            total_packet_number++;
                        }
                    }
                    else
                    {
                        total_packet_number++;
                        loss++;
                    }
                }
                else
                {
                    total_packet_number++;
                    loss++; /* Time out */
                }
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

    printf("Packet loss rate: %d\n", 100*loss/total_packet_number);
    return 0;
}

int UDP_receive_file_size(int sockfd, struct sockaddr_in addr)
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
// Usage: UDP_receive_binary_data(filename, socket_fd, filesize, sockaddr);
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

int main(int argc, char *argv[])
{
    int portnum = atoi(argv[3]);
    char *IP_address = argv[4];
    struct in_addr temp;
    struct hostent *server;
    if (strncmp(argv[4],"localhost",strlen("localhost"))==0)
    {
        server = gethostbyname(IP_address);
        bcopy((char *)server->h_addr, (char *)&temp.s_addr, server->h_length);
        IP_address = inet_ntoa(temp);
    } else {
        IP_address = argv[4];
    }

    if (argc < 5) {
        fprintf(stderr,"usage %s  TCP/UDP  SEND/RECV  Port  Hostname/IP-address  filename\n", argv[0]);
        exit(0);
    }



    if (strncmp(argv[1],"TCP",strlen("TCP"))==0)
    {
    	printf("TCP mode!\n\n");
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

		    printf("Connect success\n");


		    TCP_send_binary_data(strtok(argv[5], " "),sockfd);

		    return 0;

    	}
    	else if(strncmp(argv[2],"RECV",strlen("RECV"))==0)
    	{
		    int listenfd = 0;
		    int connfd = 0;
		    struct sockaddr_in serv_addr;
		    char sendBuff[1025];

		    listenfd = socket(AF_INET, SOCK_STREAM, 0);

		    printf("Socket retrieve success\n");

		    memset(&serv_addr, '0', sizeof(serv_addr));
		    memset(sendBuff, '0', sizeof(sendBuff));

		    serv_addr.sin_family = AF_INET;
		    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		    serv_addr.sin_port = htons(portnum);

		    bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));

		    if(listen(listenfd, 10) == -1)
		    {
		        printf("Failed to listen\n");
		        return -1;
		    }
		    connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL);


		    TCP_receive_binary_data("output_data.txt", connfd);
		    printf("Transmission is OVER!\n");

		    close(connfd);
		    return 0;

    	}
    }
    else if (strncmp(argv[1],"UDP",strlen("UDP"))==0)
    {
    	printf("UDP mode!\n\n");
    	if(strncmp(argv[2],"SEND",strlen("SEND"))==0)
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
		    addr_to.sin_port=htons(portnum);
		    addr_to.sin_addr.s_addr=inet_addr(IP_address);

		    /* Client IP Address setting*/
		    addr_from.sin_family=AF_INET;
		    addr_from.sin_port=htons(0); /* Get a free port */
		    addr_from.sin_addr.s_addr=htonl(INADDR_ANY); /* Get client-self IP address */

		    if(bind(fd,(struct sockaddr*)&addr_from,sizeof(addr_from))<0)
		    {
		        printf("Bind error!\n");
		        close(fd);
		        exit(-1);
		    }

		    printf("Client Bind successfully.\n");

		    UDP_send_binary_data(strtok(argv[5], " "),fd,addr_to);

		    close(fd);
		    return 0;
    	}
    	else if(strncmp(argv[2],"RECV",strlen("RECV"))==0)
    	{

		    struct sockaddr_in addr;
		    int listenfd,fd;

		    if((fd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
		    {
		        perror("socket create error!\n");
		        exit(-1);
		    }

		    addr.sin_family=AF_INET;
		    addr.sin_port=htons(portnum);
		    addr.sin_addr.s_addr=htonl(INADDR_ANY);

		    if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))<0)
		    {
		        printf("Bind error!\n");
		        close(fd);
		        exit(-1);
		    }
		    printf("Server bind successfully.\n");

		    struct sockaddr_in from;

		    UDP_receive_binary_data("output_data.txt", fd, from);

		    close(fd);
		    return 0;

    	}    	
    }
}
