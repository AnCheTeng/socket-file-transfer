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