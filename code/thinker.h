#ifndef THINKER_H
#define THINKER_H
#include "global.h"
#include "disk.h"
#include <cstdio>
#include <memory>


class Thinker{
public:
    Thinker();
    void swallowStatistics(){
        int StaNum = (T - 1) / FRE_PER_SLICING + 1;
        int* start = (int*)malloc(3*M*StaNum*sizeof(int));
        int** ptrStart = (int**)malloc(3*M*sizeof(int*));
        int* temp = start;
        for(int k=0;k<3;k++){
            for(int i=0;i<M;i++){
                ptrStart[i+M*k] = temp;
                temp = temp + StaNum;
            }
        }
        StatisticsBucket::initBuckData(ptrStart, ptrStart+M, ptrStart+2*M);

        for (int k=1; k <= 3; k++){
            for (int i = 1; i <= M; i++) {
                for (int j = 1; j <= StaNum; j++) {
                    scanf("%*d", &start);
                    start += 1;
                }
            }
        }
    }
    void correctPocketWatch(){
        int timestamp;
        scanf("%*s%d", &timestamp);
        int timeDelay = pocketWatch.correct(timestamp);
        printf("TIMESTAMP %d\n", pocketWatch.getTime());
        fflush(stdout);
    };

    void checkDisk(Disk disk);//更新disk的信息
private:
    PocketWatch pocketWatch;
    

};


#endif