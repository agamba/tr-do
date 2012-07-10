#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main()
{
        struct timespec ts_start;
        struct timespec ts_end;

	clock_gettime(CLOCK_MONOTONIC, &ts_start);
}
