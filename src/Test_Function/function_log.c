#include <stdio.h>
#include <time.h>

// usage: log(count, sum, total);
int log_display(int count, int sum, int total)
{
    time_t now;
    time(&now);

    if(100*sum/total > count*5)
    {
        printf("%d%%  ", count*5);
        printf("%s\n", ctime(&now));
        count+=1;
    }
    
    return count;
}