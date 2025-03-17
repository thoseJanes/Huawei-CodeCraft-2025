#include <algorithm>
#include <iostream>

int main(){

    int* unitOnDisk = (int*)malloc(5*sizeof(int));
    int objSize = 5;
    for(int i=0;i<objSize;i++){
        unitOnDisk[i] = i;
    }
    std::sort<int*>(unitOnDisk, unitOnDisk+objSize);

    for(int i=0;i<objSize;i++){
        printf("%d",unitOnDisk[i]);
    }
}
