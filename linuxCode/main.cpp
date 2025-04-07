#include <cstdio>
#include <cassert>
#include <cstdlib>
#include "global.h"
#include "worker.h"




int T, M, N, V, G, K;
void start(){
    printf("OK\n");
    LOG_IPCINFO << "\n[player]\n" << "OK\n";
    fflush(stdout);
}

void logTimeStamp(std::vector<std::tuple<std::string, int, bool>> logFiles){
    for(int i=0;i<logFiles.size();i++){
        if(std::get<2>(logFiles[i])){
            if (std::get<1>(logFiles[i]) > 0) {
                for (int j = 0; j < std::get<1>(logFiles[i]); j++) {
                    LOG_FILE(std::get<0>(logFiles[i]) + std::to_string(j)) << "\n\nTIMESTAMP " << Watch::getTime() << "\n";
                }
            }
            LOG_FILE(std::get<0>(logFiles[i])) << "\n\nTIMESTAMP " << Watch::getTime() << "\n";
        }
        
    }
};

void addLogFiles(std::vector<std::tuple<std::string, int, bool>> logFiles) {
    for (int i = 0; i < logFiles.size(); i++) {
        if (std::get<1>(logFiles[i]) > 0) {
            for (int j = 0; j < std::get<1>(logFiles[i]); j++) {
                LogFileManager::addLogFile(std::get<0>(logFiles[i]) + std::to_string(j));
            }
        }
        LogFileManager::addLogFile(std::get<0>(logFiles[i]));
    }
}

void test_numericalOverflowTest(){
    long int value = G*T*2;
    int shortValue = G*T*2;
    if(value != shortValue){
        throw std::overflow_error("G*T overflow! Affect headPlanner timeTokens calculation");
    }
}

void test_validObjectReq_testPlanner(Worker& worker){
    #ifdef ENABLE_OBJECTSCORE
    for (int i = 0; i < requestedObjects.size(); i++) {
        auto obj = requestedObjects[i];
        obj->test_validRequestsTest();
    }
    #endif
    auto manager = worker.getDiskManager();
    for (int i = 0; i < N; i++) {
        for(int j=0;j<HEAD_NUM;j++){
            auto planner = manager->getPlanner(i,j);
            planner->test_syncWithHeadTest();
            planner->test_nodeContinuousTest();
        }
    }
}

void test_bPlusTreeIteratorTest(Worker& worker) {
    static int testTimes = 10;
    testTimes -= 1;
    auto diskManager = worker.getDiskManager();
    for (int i = 0; i < diskManager->diskGroup.size(); i++) {
        auto tree = diskManager->diskGroup[i]->reqSpace;
        if (tree.getKeyNum() > 20) {
            LOG_BplusTreeTest << tree;
            
            auto iter = diskManager->diskGroup[i]->getIteratorAt(1500);
            while (!iter.isEnd()) {
                LOG_BplusTreeTest << iter.getKey();
                iter.toNext();
            }
            LOG_BplusTreeTest << "test over\n";
        }
    }
}

int main()
{
    LogFileManager::setLogFilePath("/home/eleanor-taylor/work/2025huawei-semifinals/linuxCode/log");
    //FILE* originalStdin = freopen("/home/eleanor-taylor/work/2025huawei-semifinals/data/sample_practice.in", "r", stdin);
    
    LOG_INIT << "get global params"; 
    scanf("%d%d%d%d%d%d", &T, &M, &N, &V, &G, &K);
    LOG_INIT <<"T:"<<T<<", M:"<<M<<", N:"<<N<<", V:"<<V<<", G:"<<G<<"\n";

    //文件名/序号数量/是否打印时间帧
    const std::vector<std::tuple<std::string, int, bool>> logFiles = {
    // {"main", -1, true},
    {"disk",N, true},
    {"BplusTree",N, true},
    // {"circularLinkedList",N, true},
    {"worker",-1, true},
    // {"bucketData",-1, true},
    {"request",-1, true},
    {"object",-1, true},
    {"actions",N, true},
    {"planner",N, true},
    //{"ipcInfo",-1, true},
    {"bPlusTreeTest", -1, true},
    {"score", -1, false}
    };
    //addLogFiles(logFiles);

    test_numericalOverflowTest();
    Worker worker(M);
    LOG_INIT << "get statistics";

    HistoryBucket::initBuckData();
    LOG_INIT << "init disks";
    worker.swallowStatistics();
    LOG_INIT << "start";
    LogFileManager::flushAll();
    start();
    LOG_INIT << "start loop";

    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        // if(t==1350){
        //     addLogFiles(logFiles);
        // }
        Watch::clock();
        worker.correctWatch();
        logTimeStamp(logFiles);
        worker.freshDiskTokens();
        #ifdef ENABLE_OBJECTSCORE
        worker.freshObjectScore();//必须在clearOvertimeReq之前。
        #endif
        worker.freshPhaseTwoReq();//考虑了请求超时可能有单元被取消。

        // test_validObjectReq_testPlanner(worker);
        // test_bPlusTreeIteratorTest(worker);

        worker.processDelete();
        worker.processWrite();
        worker.processRead();
        if (Watch::getTime() % FRE_PER_SLICING == 0) {
            //gc_action();
            scanf("%*s %*s");
            printf("GARBAGE COLLECTION\n");
            for (int i = 1; i <= N; i++) {
                printf("0\n");
            }
            fflush(stdout);
            LOG_SCORE<<"TIME:"<<Watch::getTime()<<", score:"
                <<Indicator::score/8<<", loss:"<<Indicator::loss/8
                <<", tol:"<<(Indicator::score - Indicator::loss)/8;
        }

    }
    //clean();

    return 0;
}
