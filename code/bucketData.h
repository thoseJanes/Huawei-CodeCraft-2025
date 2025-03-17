#ifndef BUCKETDATA_H
#define BUCKETDATA_H
#include "watch.h"
#include <memory>

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
            memcpy(start, 0, 3*M*StaNum*sizeof(int));
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
            delSta[tag][Watch::getBucket()] += num;
        }
        static void addReq(int num, int tag){
            reqSta[tag][Watch::getBucket()] += num;
        }
        static void addWrt(int num, int tag){
            wrtSta[tag][Watch::getBucket()] += num;
        }
        static int** delSta;
        static int** reqSta;
        static int** wrtSta;
    };


#endif