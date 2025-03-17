#ifndef DISK_H
#define DISK_H
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>

#include "noncopyable.h"
#include "bufferSpace.h"
#include "circularLinkedList.h"

#include "global.h"
#include "object.h"




enum HeadAction{
    JUMP,
    PASS,
    READ
};


enum UnitOrder{
    SEQUENCE,//顺序，01234
    RESERVE,//倒序， 43210
    INSERT//插序， 24130(start=size/2, interval=7(选一个大于最大size的最小质数))
};

#define INTERVAL 7

class DiskHead{
public:
    int headPos;
    HeadAction lastStatus;
    int readConsume = FIRST_READ_CONSUME;
    int presentTokens = G;
};
//磁头移动后需要更新以下两个链表
//1.存储空闲空间(位置/大小)的单向循环链表。从磁头处开始分配空间。包含磁头。第一个插入的元素是磁头。
//2.存储请求块(位置)的单向循环链表。从磁头处开始处理请求。包含磁头。
class Disk:noncopyable{
private:
    int spaceSize;
    //不仅存储对象id(int)，还存储unit值（char*),字节对齐会增加额外空间。
    //也可以用两个space，一个存char，一个存int。
    DiskHead head;
    CircularSpacePiece freeSpace;
    CircularSpaceUnit reqSpace;
    Dim1Space<int> objIdSpace;
    Dim1Space<char> untIdSpace;

    int bestHeadPos;

    //记录磁盘内各个tag占据的unit数量。
    int* tagUnitNum;//在assign这种存储对象粒度的方法中修改。而非store这种单元粒度的方法中。
    
    void allocate(int objId, std::vector<int> units, int start, int len){
        for(int i=0;i<len;i++){
            int pos = (start+len)%spaceSize;
            objIdSpace[pos] = objId;
            untIdSpace[pos] = units[i];
        }
        if(!freeSpace.testAlloc(start, len)){
            assert(false);
        }
    }
    void dealloc(int* unitOnDisk, int objSize){//高效磁盘操作很重要。建议用红黑树。也可以先缓存需要删除的节点，回合结束一并删除。
        if(objSize == 1){
            freeSpace.dealloc(unitOnDisk[0], 1);
        }else{
            std::sort<int*>(unitOnDisk, unitOnDisk+objSize);
            int i=0;
            while(i<objSize){
                int start = unitOnDisk[i];
                int len = 1;
                i+=1;
                while(i<objSize&&(unitOnDisk[i]-1==unitOnDisk[i-1])){//如果要考虑到在0处的情况，则在排序时就不应该单纯地按大小排，而应该提供排序函数。
                    len+=1;
                    i+=1;
                }
                freeSpace.dealloc(start, len);
            }
        }
    }
    std::vector<int> formUnitsByOrder(int objSize, UnitOrder unitOrder){
        std::vector<int> out = {};
        if(unitOrder == INSERT){
            assert(INTERVAL>objSize);
            int start = objSize/2;
            for(int i=0;i<objSize;i++){
                out.push_back(start%objSize);
                start += INTERVAL;
            }
        }
        else if(unitOrder == SEQUENCE){
            for(int i=0;i<objSize;i++){
                out.push_back(i);
            }
        }
        else if(unitOrder == RESERVE){
            for(int i=objSize-1;i>=0;i--){
                out.push_back(i);
            }
        }
        else{
            throw std::out_of_range("unitOrder type not exists");
        }
        return out;
    }
    
public:
    Disk(int spacesize, int headPos = 0):freeSpace(spacesize){
        this->spaceSize = spacesize;
        this->head.headPos = headPos;
        objIdSpace.initSpace(spacesize);
        untIdSpace.initSpace(spacesize);

        tagUnitNum = (int*)malloc(sizeof(int)*M);
        memcpy(tagUnitNum, 0, sizeof(int)*M);//初始化。
    }
    
    void freshTokens(){this->head.presentTokens = G;}
    // //可以尝试建表规划？
    // int testConsume(HeadAction action, int times){
    //     //返回操作消耗的令牌数,只是测试时间花费，不会改变状态。
    //     if(action == READ){
    //         int tolConsume = 0;
    //         for(int i=0;i<times;i++){
    //             int tempReadConsum = head.readConsume;
    //             tolConsume += tempReadConsum;
    //             tempReadConsum = std::max<int>(16, ceil(0.8*head.readConsume));
    //         }
    //         return tolConsume;
    //     }else if(action == PASS){
    //         return times;
    //     }else if(action == JUMP){
    //         return G;
    //     }
    // }
    int excute(HeadAction action, int times);
    
    //urgent表明该对象写入时即需要读取。tag表明是否按tag尽量分在同一tag周围（或相关性较大tag周围）。
    int assignSpace(int objSize ,UnitOrder order ,bool urgent, int tag = -1){
        std::vector<int> units = formUnitsByOrder(objSize, order);////////////////////////
        const SpacePieceNode* node = freeSpace.getStartAfter(head.headPos, true);
        //虽然头数据知道对象数量，但不知道对象大小。
        //可以规划分区。
        //可以通过小size对象来减小磁盘碎片。
        //在空的地方读会报错吗？





        this->tagUnitNum[tag] += objSize;
    }
    int releaseSpace(int* unitOnDisk, int objSize, int tag){
        this->dealloc(unitOnDisk, objSize);
        this->tagUnitNum[tag] -= objSize;
    }
    
    int addReqUnit(int unit){
        reqSpace.addReqUnit(unit);
    }
    int rmReqUnit(int unit){
        reqSpace.rmReqUnit(unit);
    }
    

    int getFreeSpaceSize(){return freeSpace.getTolSpace();}
    int getBestHeadPos(int timeStamp){
        return 0;
    }
    
    ~Disk(){
        free(tagUnitNum);
    }

    /*
    //有几种可能：
    
    //1.写入的同时即需要读取。——分配到磁盘紧前，如果短距离可以与已有空间相邻则相邻，否则直接分配。
    //2.写入时不需要读取，但是该时间桶和下个时间桶内读取的概率较高。分配到紧前与已有空间相邻处。
    //3.写入同时不需读取，且下面时间桶内读取概率较低。分配到与已有较大空间相邻处。
    //分配原则：
    //1.同一tag最好分到紧邻空间，实现连续读。
    //2.同一tag可以轮转磁盘分配，实现负载均衡（负载均衡指同一时期不同磁盘的均衡，而非不同时期同一磁盘的平均）
    //3.计算不同tag间的相关系数，相关系数大者可以分配到靠近位置，正偏移相关分到紧前，负偏移相关分到紧后。
   
        由于最快可以4倍读，减少整体开销，因此可以原则1优先，也可以原则2优先，分配优先系数。
    */

    //磁头策略：
    /*
    局部路径优化策略：
        首先通过算法得到副本选择。然后检查以下项以进行优化：
        1.移动改为连读能否取得更大效益
        2.
    剩余时间策略：
        当前时间片磁头读取后若剩余若干时间片————
            1.当前位置为空，则直接移动
            2.当前时间片磁头读取后若剩余若干时间片，且当前位置非空，则通过判断磁头前方是否有需要读取内容确定是否继续移动。
                a.磁头前方一定步数有需要读取内容。如果连读给下个时间片带来的效益大于不连读，则连读，否则不连读
            3.如果没有，则判断磁头位置是否最佳（概率意义上）
        当前时间片磁头无读取项————
            1.判断磁头位置是否最佳（概率意义上）。找到最佳位置并用一个时间片移动。
    */

    //读取策略
    /*
        如果以不放弃每一个对象的方式，且不回读，那么可以简化很多策略。
        每次取十个磁头最靠近的十个请求，规划是否选择这十个请求中的副本。
    */
};

class DiskManager{
public:
    std::vector<Disk*> diskGroup;
public:
    //存储一个活动对象的id索引。方便跟进需要查找的单元信息。
    DiskManager(){};
    void addDisk(int spaceSize){
        auto diskptr = new Disk(spaceSize);
        diskGroup.push_back(diskptr);
    }
    ~DiskManager(){
        for(int i=0;i<diskGroup.size();i++){
            delete diskGroup[i];
        }
    }
    void freshNewReqUnits(const Object& obj){
        for(int u=0;u<obj.size;u++){//对不同unit遍历。
            if(obj.unitReqNum[u]==1){//说明是新增的(从0到1)unit需求
                for(int i=0;i<REP_NUM;i++){
                    int diskId = obj.replica[i];
                    diskGroup[diskId]->addReqUnit(obj.unitOnDisk[i][u]);
                }
            }
        }
    }
    void freshOvertimeReqUnits(const Object& obj, std::vector<int> unitPos){
        for(int r=0;r<REP_NUM;r++){//第几个副本
            Disk* disk = this->diskGroup[obj.replica[r]];
            for(int i=0;i<unitPos.size();i++){
                disk->rmReqUnit(obj.unitOnDisk[r][i]);
            }
        }
    }
    
    void assignSpace(Object& obj){
        //首先挑出3块空间够且负载低的磁盘，然后选择存储位置，然后选择存储顺序。原则如下：
        /*
            存储位置：同一对象分配的空间尽量连续（方便连读）。
            存储顺序：三个副本尽量采用不同的存储顺序。
        */


        
    }
    std::array<std::vector<std::pair<HeadAction, int>>, 10> planHeadMove(std::vector<int>& doneReqIds){//输出10个磁盘的一个时间步的行动。
        //plan


        //doneRequests = obj.commitUnit()，得到已完成的请求id。

    }
    void freeSpace(Object& obj){
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj.replica[i];
            Disk* disk = diskGroup[diskId];
            disk->releaseSpace(obj.unitOnDisk[i], obj.size, obj.tag);
        }
    }
    
    void freshTokens(){
        for(int i=0;i<diskGroup.size();i++){
            diskGroup[i]->freshTokens();
        }
    }

};

#endif