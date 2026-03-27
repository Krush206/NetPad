#include <ctype.h>

int parsePort(char *port)
{
    while(*port != '\0')
        if(!isdigit(*port++))
            return 1;
    return 0;
}
