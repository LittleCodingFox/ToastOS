#include <stdio.h>
#include <time.h>

int main(int argc, char **argv)
{
    time_t timeBase;
    struct tm *timeInfo;

    time(&timeBase);

    timeInfo = localtime(&timeBase);

    printf("Current time: %s", asctime(timeInfo));

    return 0;
}
