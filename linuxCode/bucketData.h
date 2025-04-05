#ifndef BUCKETDATA_H
#define BUCKETDATA_H
#include "watch.h"
#include <memory>
#include <cstdlib>
#include <algorithm>
#define LOG_BUCKETDATA LOG_FILE("bucketData")
class StatisticsBucket;
typedef StatisticsBucket StaBkt;
class StatisticsBucket{
    public:
        static  int*  * delSta;
        static  int*  * reqSta;
        static  int*  * wrtSta;
        static double* rltSta;
        static int** leftReqs;
        static int staNum;

        static void initStaNum(int value){
            staNum = value;
        }
        static void initBuckData(int** delsta, int** wrtsta, int** reqsta){
            delSta = delsta;reqSta = reqsta;wrtSta = wrtsta;
            initReqRelativity();
            initLeftReqs();
        }
        static void initReqRelativity(){
            //计算相关性
            StaBkt::rltSta = (double*)malloc(sizeof(double)*M*M);
            for (int i = 0; i < M; i++) {
                StaBkt::rltSta[i * M + i] = 0;//请求总数。
                for (int j = 0; j < StaBkt::staNum; j++) {
                    StaBkt::rltSta[i * M + i] += StaBkt::reqSta[i][j];
                }
            }
            for (int i1 = 0; i1 < M; i1++) {
                for (int i2 = 0; i2 < i1; i2++) {
                    StaBkt::rltSta[i1 * M + i2] = 0;
                    for (int j = 0; j < StaBkt::staNum; j++) {
                        StaBkt::rltSta[i1 * M + i2] += 1.0* StaBkt::reqSta[i1][j] * StaBkt::reqSta[i2][j];
                    }
                    StaBkt::rltSta[i1 * M + i2] /= 
                        (1.0 * StaBkt::rltSta[i1 * M + i1] * StaBkt::rltSta[i2 * M + i2]);
                    StaBkt::rltSta[i2 * M + i1] = StaBkt::rltSta[i1 * M + i2];
                }
            }
        }
        static void initLeftReqs(){
            int* leftReqsValue = (int*)malloc(M*StaBkt::staNum*sizeof(int));
            StaBkt::leftReqs = (int**)malloc(M*sizeof(int*));
            int* temp = leftReqsValue;
            for(int i=0;i<M;i++){
                StaBkt::leftReqs[i] = temp;
                temp = temp + StaBkt::staNum;
            }
            for(int i=0;i<M;i++){
                StaBkt::leftReqs[i][0] = StaBkt::getRelativity(i, i);//总tag数
                for(int j=1;j<StaBkt::staNum;j++){
                    StaBkt::leftReqs[i][j] = StaBkt::leftReqs[i][j-1] - StaBkt::reqSta[i][j-1];
                }
            }
        }

        static double getRelativity(int tagi, int tagj) {
            return rltSta[tagi * M + tagj];
        }
        static int getMaxRltTag(int tag, const std::vector<int>& tagsExcluded) {
            int maxRltTag = 0;
            double maxRltVal = 0;
            for (int i = 0; i < M; i++) {
                if (std::find(tagsExcluded.begin(), tagsExcluded.end(), i) == tagsExcluded.end()) {//如果i没有在tagIn中
                    double rlt = rltSta[i * M + tag];
                    if (rlt > maxRltVal) {
                        maxRltVal = rlt;
                        maxRltTag = i;
                    }
                }
            }
            return maxRltTag;
        }
        static int getMinRltTag(int tag, const std::vector<int>& tagsExcluded) {
            int minRltTag = 0;
            int minRltVal = 1;
            for (int i = 0; i < M; i++) {
                if (std::find(tagsExcluded.begin(), tagsExcluded.end(), i) == tagsExcluded.end()) {//如果i没有在tagIn中
                    int rlt = rltSta[i * M + tag];
                    if (rlt < minRltVal) {
                        minRltVal = rlt;
                        minRltTag = i;
                    }
                }
            }
            return minRltTag;
        }
        static int getPresentReqLeft(int tag){
            return StaBkt::leftReqs[tag][Watch::getBucket()];
        }
};
    
    //历史数据，可以用来判断接下来的时间内请求或其它出现的概率。
    //但是，当以这种方式计算概率时，越接近桶边缘，则计算的概率有效期越短。
class HistoryBucket{
    public:
        static void initBuckData(){
            int StaNum = (T - 1) / FRE_PER_SLICING + 1;
            int* start = (int*)malloc(3*M*StaNum*sizeof(int));
            for(int i=0;i<3*M*StaNum;i++){
                start[i] = 0;
            }
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
            LOG_BUCKETDATA << "bucket:"<<Watch::getBucket()<<", tag:"<<tag<<", num:"<<num;
            delSta[tag][Watch::getBucket()] += num;
            LOG_BUCKETDATA << "add del over";
        }
        static void addReq(int num, int tag){
            LOG_BUCKETDATA << "bucket:"<<Watch::getBucket()<<", tag:"<<tag<<", num:"<<num;
            reqSta[tag][Watch::getBucket()] += num;
            LOG_BUCKETDATA << "add req over";
        }
        static void addWrt(int num, int tag){
            LOG_BUCKETDATA << "bucket:"<<Watch::getBucket()<<", tag:"<<tag<<", num:"<<num;
            wrtSta[tag][Watch::getBucket()] += num;
            LOG_BUCKETDATA << "add wrt over";
        }
        static int** delSta;
        static int** reqSta;
        static int** wrtSta;
    };


#endif
