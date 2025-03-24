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
    auto bpTree = new BplusTree<10>;
    for(int i=30;i<500;i+=10){
        bpTree->insert(i);
        printf("insert\n");
        LOG_BplusTree << *bpTree;
    }
    LOG_BplusTree<<"begin remove\n";
    for(int i=50-1;i>40;i-=10){
        bpTree->remove(i);
        LOG_BplusTree << *bpTree;
        LogFileManager::flushAll();
    }
    
    bpTree->setAnchor(432);
    LOG_BplusTree << *bpTree;
    LOG_BplusTree << "next anchor" << bpTree->getNextKeyByAnchor();
    LOG_BplusTree << "next anchor" << bpTree->getNextKeyByAnchor();
    LOG_BplusTree << "next anchor" << bpTree->getNextKeyByAnchor();
    LOG_BplusTree << "next anchor" << bpTree->getNextKeyByAnchor();
    LOG_BplusTree << "begin remove 20-30\n";
    for (int i = 20; i <30; i++) {
        if(i==25){
            int k = 0;
        }
        bpTree->insert(i);
        LOG_BplusTree << *bpTree;
        LogFileManager::flushAll();
    }
    bpTree->setAnchor(23, true);
    LOG_BplusTree << *bpTree;
    LOG_BplusTree << "next anchor" << bpTree->getNextKeyByAnchor();
    for (int i = 0; i < 10; i++) {
        if (i == 4) {
            int k = 0;
        }
        bpTree->insert(i);
        LOG_BplusTree << *bpTree;
        LogFileManager::flushAll();
    }
    printf("remove over\n");
    return 0;
}
