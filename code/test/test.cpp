#include <algorithm>
#include <iostream>
#include <memory>
#include "bplusTree.h"


int main(){
    printf("start\n");
    LogFileManager::setLogFilePath("./log");
    LogFileManager::addLogFile("BplusTree");
    LogFileManager::addLogFile("BplusTreeInfo");
    LOG_FILE("BplusTree") << "some info";
    LogFileManager::flushAll();
    std::atexit([](){
        printf("wait flush\n");
        LogFileManager::flushAll();
    });
    printf("begin\n");
    auto bpTree = new BplusTree<4>;
    for(int i=0;i<50;i++){
        bpTree->insert(i);
        printf("insert\n");
        LOG_BplusTree << *bpTree;
    }
    printf("begin remove\n");
    for(int i=50-1;i>40;i--){
        bpTree->remove(i);
        LOG_BplusTree << *bpTree;
        LogFileManager::flushAll();
    }
    for (int i = 20; i <30; i++) {
        bpTree->remove(i);
        LOG_BplusTree << *bpTree;
        LogFileManager::flushAll();
    }
    for (int i = 0; i < 10; i++) {
        bpTree->remove(i);
        LOG_BplusTree << *bpTree;
        LogFileManager::flushAll();
    }
    printf("remove over\n");
    return 0;
}
