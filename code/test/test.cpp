#include <algorithm>
#include <iostream>
#include <memory>
#include "bplusTree.h"


int main(){
    LogFileManager::setLogFilePath("./log");
    LogFileManager::addLogFile("BplusTree");
    LogFileManager::addLogFile("BplusTreeInfo");
    auto bpTree = new BplusTree<4>;
    for(int i=0;i<30;i++){
        bpTree->insert(i);
    }
}
