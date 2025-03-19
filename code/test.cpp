#include <algorithm>
#include <iostream>
#include <memory>

int main(){

    int* unitOnDisk = (int*)malloc(5*sizeof(int));
    int objSize = 5;
    for(int i=0;i<objSize;i++){
        unitOnDisk[i] = i;
    }
    std::sort<int*>(unitOnDisk, unitOnDisk+objSize);
    memmove(unitOnDisk+1, unitOnDisk, 4*sizeof(int));

    for(int i=0;i<objSize;i++){
        printf("%d",unitOnDisk[i]);
    }
}
