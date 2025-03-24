#include <algorithm>
#include <iostream>
#include <memory>
#include "circularLinkedList.h"
#include "Logger.h"

CircularSpacePiece freeSpace(3000);

int main(){
    printf("start\n");
    LogFileManager::setLogFilePath("./log");
    LogFileManager::addLogFile("circularLinkedList");
    LogFileManager::flushAll();
    std::atexit([](){
        printf("wait flush\n");
        LogFileManager::flushAll();
    });
    printf("begin\n");
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.testAlloc(8, 14);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.testAlloc(22, 100);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.testAlloc(250, 800);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.testAlloc(122, 128);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.testAlloc(1050, 2998-1050);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.testAlloc(2999, 2);
    LOG_LINKEDSPACE << freeSpace;

    freeSpace.dealloc(9, 2);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.dealloc(12, 3);
    LOG_LINKEDSPACE << freeSpace;
    //freeSpace.dealloc(7, 2);
    //LOG_LINKEDSPACE << freeSpace;
    freeSpace.dealloc(2999, 2);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.dealloc(8, 1);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.dealloc(11, 1);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.testAlloc(0, 2);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.dealloc(0, 2);
    LOG_LINKEDSPACE << freeSpace;
    freeSpace.dealloc(15, 2997);
    LOG_LINKEDSPACE << freeSpace;
    return 0;
}
