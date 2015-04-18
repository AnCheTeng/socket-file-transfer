#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

/* Function: server_dir(DIR *mydir, char* dir)
   mydir:
*/
int server_dir(DIR *mydir, char* dir)
{
    struct dirent *myfile;
    struct stat mystat;
    char buf[512];
    // myfile = readdir(mydir) success, then will return next dir-node
    while((myfile = readdir(mydir)) != NULL)
    {
        sprintf(buf, "%s/%s", dir, myfile->d_name);
        stat(buf, &mystat);
        printf("%d %s\n",mystat.st_size, myfile->d_name);
    }
    return 0;
}

// execution
int main(int argc, char* argv[])
{
    DIR *mydir;
    mydir = opendir(argv[1]);
    server_dir(mydir, argv[1]);
    closedir(mydir);
}
