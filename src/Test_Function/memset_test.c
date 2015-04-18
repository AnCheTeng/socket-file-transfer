#include <stdio.h>
#include <string.h>
 
int main(void)
{
    char s[] = "congratulation";
     
    printf("%s\n", memset(s, 'n', 5));
     
    return 0;   
}