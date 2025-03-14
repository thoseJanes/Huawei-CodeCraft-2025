#ifndef DISK_H
#define DISK_H
#include <stdexcept>
#include <memory>
#include "noncopyable.h"
#include "global.h"
#include <vector>
#include "bufferSpace.h"



class Disk:noncopyable{
private:
    int unitNum;
    int headPos;
    Dim1Space space;//直接用array？
    int bestHeadPos;
public:
    Disk(int unitNum, int headPos = 0){
        this->unitNum = unitNum;
        this->headPos = headPos;
        space.initSpace(unitNum);
    }
    int bestHeadPos(int timeStamp){
        
    }
};

class DiskManager{
public:
    DiskManager(){};
    void addDisk(Disk* diskptr){
        diskGroup.push_back(diskptr);
    }
    ~DiskManager(){
        for(int i=0;i<diskGroup.size();i++){
            delete diskGroup[i];
        }
    }
    void assignSpace(StorageObject& obj){
        //分配空间策略
    }
private:
    std::vector<Disk*> diskGroup;
};

#endif