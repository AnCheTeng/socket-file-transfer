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
