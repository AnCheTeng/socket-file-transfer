#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

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
    //printf("Here is the message: %s",buff);
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

        //printf("Bytes read %d. ", nread);        
        
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

int main(void)
{
    int listenfd = 0;
    int connfd = 0;
    struct sockaddr_in serv_addr;
    char sendBuff[1025];
    int numrv;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    printf("Socket retrieve success\n");

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(50000);

    bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));

    if(listen(listenfd, 10) == -1)
    {
        printf("Failed to listen\n");
        return -1;
    }

    connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL);

    //===========================================================
    int flag;
    flag = receive_request(connfd);
    while (flag!=20 && flag!=15)
    {
        system_send_request(connfd,"WRONG");
        flag = receive_request(connfd);
    }
    //===========================================================
    
    if(flag==20)
    {
        system_send_request(connfd,"SOK");
    } else if(flag==15)
    {
        system_send_request(connfd,"ROK");
    }

    if (flag == 15)
    {
        /* if server want to send data: */
        send_binary_data("sample_file1.txt",connfd);        
    } else if (flag == 20)
    {
        /* if server want to receive data: */
        receive_binary_data("sample_file2.txt", connfd, 13999);
    }

    close(connfd);
    return 0;
}