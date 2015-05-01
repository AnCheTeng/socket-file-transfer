#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
    char s[] = "0 Speech is si1ver, silence is gold.";
    char *ack = strtok(s, " ");
    printf("%s\n", ack);
    char *msg = strtok(NULL, ""); 
    printf("%s\n", msg);
    char pack[256] = "0 ";
    char *packet = strcat(pack,msg);
    printf("%s\n", packet);
    // printf("%d\n", strlen(packet));
    return 0;    
}