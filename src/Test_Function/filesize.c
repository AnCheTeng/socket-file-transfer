#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

/* Function: server_dir(DIR *mydir, char* dir)
   mydir:
*/
int filesize(FILE *fp)
{
    int sz;
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    return sz;
}

// execution
int main(int argc, char* argv[])
{
    FILE *fp = fopen(argv[1],"rb");
    if(fp==NULL)
    {
        printf("File opern error.\nUsage: ./filesize filename\n");
        return 1;   
    }   

    printf("Size of chapter4a.pdf: %d\n",filesize(fp));
}
