#pragma once
#include <stdio.h>
#include <string.h>


int inline timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);

    fflush(stdout);
    return timestamp;
}