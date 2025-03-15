#ifndef GLOBAL_H
#define GLOBAL_H

#include "pocketWatch.h"

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)//4字节（32位）：范围是 [−2147483648,2147483647]
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)

extern int T, M, N, V, G;
class StatisticsBucket{
public:
    static void initBuckData(int** delsta, int** reqsta, int** wrtsta){
        delSta = delsta;reqSta = reqsta;wrtSta = wrtsta;
    }
    static const int* const * delSta;
    static const int* const * reqSta;
    static const int* const * wrtSta;
};

//历史数据，可以用来判断接下来的时间内请求或其它出现的概率。
//但是，当以这种方式计算概率时，越接近桶边缘，则计算的概率有效期越短。
class HistoryBucket{
public:
    static void initBuckData(){
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
        delSta = ptrStart;
        reqSta = ptrStart + M;
        wrtSta = ptrStart + 2*M;
    }
    static void addDel(int num, int tag){
        delSta[tag][PocketWatch::getBucket(FRE_PER_SLICING)] += num;
    }
    static void addReq(int num, int tag, int time){
        reqSta[tag][PocketWatch::getBucket(FRE_PER_SLICING)] += num;
    }
    static void addWrt(int num, int tag, int time){
        wrtSta[tag][PocketWatch::getBucket(FRE_PER_SLICING)] += num;
    }
    static int** delSta;
    static int** reqSta;
    static int** wrtSta;
};
//模糊模式：对象数量并不精准契合统计数据
//精准模式：对象数量完美契合统计数据

#endif // GLOBAL_H



