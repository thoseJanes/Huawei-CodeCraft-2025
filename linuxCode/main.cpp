#include <cstdio>
#include <cassert>
#include <cstdlib>
#include "global.h"
#include "worker.h"

#define LOG_INIT LOG_FILE("main")


int T, M, N, V, G;
void start(){
    printf("OK\n");
    LOG_IPCINFO << "\n[player]\n" << "OK\n";
    fflush(stdout);
}

void logTimeStamp(std::vector<std::pair<std::string, int>> logFiles){
    for(int i=0;i<logFiles.size();i++){
        if (logFiles[i].second > 0) {
            for (int j = 0; j < logFiles[i].second; j++) {
                LOG_FILE(logFiles[i].first + std::to_string(j)) << "\n\nTIMESTAMP " << Watch::getTime() << "\n";
            }
        }
        LOG_FILE(logFiles[i].first) << "\n\nTIMESTAMP " << Watch::getTime() << "\n";
    }
};

void addLogFiles(std::vector<std::pair<std::string, int>> logFiles) {
    for (int i = 0; i < logFiles.size(); i++) {
        if (logFiles[i].second > 0) {
            for (int j = 0; j < logFiles[i].second; j++) {
                LogFileManager::addLogFile(logFiles[i].first + std::to_string(j));
            }
        }
        LogFileManager::addLogFile(logFiles[i].first);
    }
}

void test_numericalOverflowTest(){
    long int value = G*T*2;
    int shortValue = G*T*2;
    if(value != shortValue){
        throw std::overflow_error("G*T overflow! Affect headPlanner timeTokens calculation");
    }
}

int main()
{
    //LogFileManager::setLogFilePath(".\\log");

    FILE* originalStdin = freopen("E:\\work\\study\\affair_AfterUndergraduate\\competitions\\2025huawei\\data\\sample_practice.in", "r", stdin);
    
    LOG_INIT << "get global params"; 
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    LOG_INIT <<"T:"<<T<<", M:"<<M<<", N:"<<N<<", V:"<<V<<", G:"<<G<<"\n";
    test_numericalOverflowTest();
    Worker worker;
    LOG_INIT <<"get statistics";
    
    HistoryBucket::initBuckData();
    worker.swallowStatistics();
    LOG_INIT <<"init disks";
    worker.initDisk();
    LOG_INIT <<"start";
    LogFileManager::flushAll();
    start();
    LOG_INIT <<"start loop";

    const std::vector<std::pair<std::string, int>> logFiles = {
        {"main", -1},
        {"disk",N},
        {"BplusTree",N},
        {"circularLinkedList",N},
        {"worker",-1},
        {"bucketData",-1},
        {"request",-1},
        {"object",-1},
        {"actions",N},
        {"ipcInfo",-1},
    };
    
    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        //if (t == 1) {
        //    addLogFiles(logFiles);
        //}
        Watch::clock();
        worker.correctWatch();
        //logTimeStamp(logFiles);
        worker.freshDiskTokens();
        worker.freshObjectScore();//必须在clearOvertimeReq之前。
        worker.clearOvertimeReqAndFreshPhaseTwoReq();//考虑了请求超时可能有单元被取消。

        for(int i=0;i<requestedObjects.size();i++){
            auto obj = requestedObjects[i];
            obj->test_validRequestsTest();
        }
        for(int i=0;i<N;i++){
            auto manager = worker.getDiskManager();
            auto planner = manager->getPlanner(i);
            planner->test_syncWithHeadTest();
        }

        worker.processDelete();
        worker.processWrite();
        worker.processRead();
    }
    //clean();

    return 0;
}