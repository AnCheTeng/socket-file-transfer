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
