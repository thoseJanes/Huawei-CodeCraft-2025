#ifndef DISK_H
#define DISK_H
#include <stdexcept>
#include <memory>
#include "noncopyable.h"
#include "global.h"
#include <vector>
#include "bufferSpace.h"
#include "circularLinkedList.h"

//磁头移动后需要更新以下两个链表
//1.存储空闲空间(位置/大小)的单向循环链表。从磁头处开始分配空间。包含磁头。第一个插入的元素是磁头。
//2.存储请求块(位置)的单向循环链表。从磁头处开始处理请求。包含磁头。
class Disk:noncopyable{
private:
    int unitNum;
    int headPos;
    Dim1Space<int> objIdSpace;
    Dim1Space<char> untIdSpace;
    //不仅存储对象id(int)，还存储unit值（char*),字节对齐会增加额外空间。
    //也可以用两个space，一个存char，一个存int。
    CircularSpacePiece freeSpace;
    CircularSpaceUnit reqSpace;
    int bestHeadPos;
public:
    Disk(int unitNum, int headPos = 0){
        this->unitNum = unitNum;
        this->headPos = headPos;
        objIdSpace.initSpace(unitNum);
        untIdSpace.initSpace(unitNum);
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
        //首先挑出3块空间够且负载低的磁盘，然后选择存储位置，然后选择存储顺序。原则如下：
        /*
            存储位置：同一对象分配的空间尽量连续（方便连读）。
            存储顺序：三个副本尽量采用不同的存储顺序。
        */
        
    }
private:
    std::vector<Disk*> diskGroup;
};

#endif