#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>

// usage: log(count, sum, total);
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

int system_send_request(int connfd, char *ack)
{
    write(connfd, ack, strlen(ack));
    return(0);
}


int send_request(int connfd)
{
    char buff[256];
    memset(buff, 0, sizeof(buff));
    fgets(buff,255,stdin);
    write(connfd, buff, strlen(buff));
    return(0);
}

int receive_request(int sockfd)
{
    char buff[256];
    memset(buff, 0, sizeof(buff));
    read(sockfd,buff,255);
    //printf("Here is the message: %s\n",buff);
    if (strncmp(buff,"SEND",4)==0)
        return 20;
    else if (strncmp(buff,"REQUEST",7)==0)
        return 15;
    else if (strncmp(buff,"SOK",3)==0)
        return 10;
    else if (strncmp(buff,"ROK",3)==0)
        return 5;
    else
        return 0;
}

int send_binary_data(char* filename, int connfd)
{

    /* Open the file that we wish to transfer */
    FILE *fp = fopen(filename,"rb");
    if(fp==NULL)
    {
        printf("File opern error");
        return 1;   
    }   

    /* Read data from file and send it */
    while(1)
    {
        /* First read file in chunks of 256 bytes */
        unsigned char buff[256]={0};
        int nread = fread(buff,1,256,fp);

        //====================== Log function usage ======================
        //printf("Bytes read %d. ", nread);   
        //====================== Log function usage ======================
        
        /* If read was success, send data. */
        if(nread > 0)
        {
            //printf("Sending. \n");
            write(connfd, buff, nread);
        }
        /*
         * There is something tricky going on with read .. 
         * Either there was error, or we reached end of file.
         */
        if (nread < 256)
        {
            if (feof(fp))
                printf("End of file\n");
            if (ferror(fp))
                printf("Error reading\n");
            break;
        }
    }

    return 0;
}

int receive_binary_data(char* filename, int sockfd, int total)
{
    /* Create file where data will be stored */
    FILE *fp;
    int bytesReceived = 0;
    char recvBuff[256];
    memset(recvBuff, '0', sizeof(recvBuff));

    int sum = 0;
    int count = 0;

    //fp = fopen(filename, "ab");
    fp = fopen(filename,"wb");
    if(NULL == fp)
    {
        printf("Error opening file");
        return 1;
    }

    /* Receive data in chunks of 256 bytes */
    while((bytesReceived = read(sockfd, recvBuff, 256)) > 0)
    {

        //====================== Log function usage ======================
        sum += bytesReceived;
        count = log_display(count, sum, total);
        //printf("Bytes received %d\n",bytesReceived);
        //====================== Log function usage ======================

        // recvBuff[n] = 0;
        fwrite(recvBuff, 1,bytesReceived,fp);
        // printf("%s \n", recvBuff);
    }
    count+=1;
    log_display(count, sum, total);
    if(bytesReceived < 0)
    {
        printf("\n Read Error \n");
    }

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
        printf("Local ");
        server = gethostbyname(IP_address);
        bcopy((char *)server->h_addr, (char *)&temp.s_addr, server->h_length);
        IP_address = inet_ntoa(temp);
    } else {
        printf("Remote ");
        IP_address = argv[4];
    }


    if (argc < 5) {
    fprintf(stderr,"usage %s TCP/UDP Client/Server Port Hostname/IP-address\n", argv[0]);
    exit(0);
    }


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

    //===========================================================
    printf("Please enter 'SEND' or 'REQUEST'\n");
    send_request(sockfd);
    int flag;
    flag = receive_request(sockfd);
    while (flag!=10 && flag!=5)
    {
        printf("Please enter 'SEND' or 'REQUEST'\n");
        send_request(sockfd);
        flag = receive_request(sockfd);
    }
    //===========================================================

    if (flag == 10)
    {
        /* if client want to send data: */    
        send_binary_data("sample_file1.txt",sockfd);      
    } else if (flag == 5)
    {
        /* if client want to receive data: */
        receive_binary_data("sample_file2.txt", sockfd, 13999);
    }

    //?????close socket(WTF)
    system_send_request(sockfd,"");
    return 0;
}