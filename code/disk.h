#ifndef DISK_H
#define DISK_H
#include <stdexcept>
#include <memory>
#include "noncopyable.h"
#include <vector>

class CircleSpace{
public:
    void initSpace(int size){
        buffer = (int*)malloc(sizeof(int)*size);
    }
    int& operator[](int index){
        index = index%bufLen;
        return *(buffer+index);
    }
    ~CircleSpace(){
        free(buffer);
    }
private:
    int* buffer = nullptr;
    int bufLen = 0;
};

class Disk{
private:
    int unitNum;
    int headPos;
    CircleSpace space;
public:
    Disk(int unitNum, int headPos = 0){
        this->unitNum = unitNum;
        this->headPos = headPos;
        space.initSpace(unitNum);
    }

};

class DiskGroup{
public:
    DiskGroup(){};
    void addDisk(Disk* diskptr){
        diskGroup.push_back(diskptr);
    }
    Disk* operator[](int index){
        if(index<0||index>diskGroup.size()){
            throw std::out_of_range("disk out of range!");
        }
        return diskGroup[index];
    }
    ~DiskGroup(){
        for(int i=0;i<diskGroup.size();i++){
            delete diskGroup[i];
        }
    }
private:
    std::vector<Disk*> diskGroup;
};


#endif