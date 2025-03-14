#ifndef THINKER_H
#define THINKER_H
#include "global.h"
#include "disk.h"
#include <cstdio>
#include <memory>

class PocketWatch{
public:
    int correct(int timeStamp){
        int offset = timeStamp - timeStamp_;
        if(offset){timeStamp_ += offset; return offset;}
        return 0;
    }
    int getTime(){return timeStamp_;}
    static void clock(){timeStamp_++;}
private:
    static int timeStamp_;
};
int PocketWatch::timeStamp_ = 0;

class Thinker{
public:
    Thinker();
    void swallowStatistics(){
        int StaNum = (T - 1) / FRE_PER_SLICING + 1;
        int* start = (int*)malloc(3*StaNum*M*sizeof(int));
        delSta = start;
        reqSta = delSta + StaNum*M;
        wrtSta = reqSta + StaNum*M;
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
    int timeDelay;
    int* delSta;
    int* reqSta;
    int* wrtSta;
};


#endif