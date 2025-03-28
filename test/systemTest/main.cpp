#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;
int main(){
    #ifdef WIN32
    printf("win32\n");
    #endif
    #ifdef _LINUX_
    printf("sdfa\n");
    #endif

    return 0;
}