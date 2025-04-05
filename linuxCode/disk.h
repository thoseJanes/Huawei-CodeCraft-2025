#ifndef DISK_H
#define DISK_H
#include <stdexcept>
#include <memory>
#include <algorithm>

#include <vector>
#include <array>
#include <list>

#include "bufferSpace.h"
#include "circularLinkedList.h"
#include "bucketData.h"

#include "global.h"
#include "object.h"
#include "bplusTree.h"

#define LOG_DISK LOG_FILE("disk")
#define LOG_ACTIONS LOG_FILE("actions")
#define LOG_DISKN(x) LOG_FILE("disk"+std::to_string(x))
#define LOG_ACTIONSN(x) LOG_FILE("actions"+std::to_string(x))

class HeadOperator;
LogStream& operator<<(LogStream& s, const HeadOperator& headOperator);
LogStream& operator<<(LogStream& s, HeadOperator& headOperator);

enum HeadAction {
    NONE,//只用在磁头的toBeComplete中。
    START,//只用在HeadPlanner中。作为planner的起始节点。相应参数为AheadRead
    JUMP,//HeadOperator参数：jumpTo，跳到的位置
    PASS,//HeadOperator参数：passTimes，连续pass的步数
    READ,//HeadOperator参数：aheadRead，前面已经读过的步数。
    VREAD//只读不报。适用于无效读。
};
struct HeadOperator{
    HeadAction action;
    union{
        int aheadRead;//之前的读操作数量。
        int passTimes;//pass的次数。
        int jumpTo;//跳到的位置。

        int param;//指参数。只需要比较相同action的参数值时使用。
    };
};
enum UnitOrder{
    SEQUENCE,//顺序，01234
    RESERVE,//倒序， 43210
    INSERT//插序， 24130(start=size/2, interval=7(选一个大于最大size的最小质数))
};

#define INTERVAL 7
class DiskHead{
private:
    
public:
    int diskId;//为了log而加的
    const int headId;
    const int spaceSize;
    int headPos = 0;
    int readConsume = FIRST_READ_CONSUME;
    int presentTokens = G;
    int tokensOffset = 0;//在上个回合花的下个回合的令牌。
    HeadOperator toBeComplete = {NONE, 0};
    DiskHead(int id, int spacesize):headId(id),spaceSize(spacesize){LOG_DISK<< "init head";}
    void freshTokens(){
        this->presentTokens = G - this->tokensOffset;
        this->tokensOffset = 0;
    }
    int calTokensCost(HeadOperator headOperator){
        auto action = headOperator.action;
        if(action == READ || action ==VREAD){
            assert(headOperator.aheadRead == getAheadReadTimes(readConsume));
            int consume = readConsume;
            readConsume = getNextReadConsume(readConsume);
            return consume;
        }else if(action == PASS){
            return headOperator.passTimes;
        }else if(action == JUMP){
            return G;
        }
    }
    //只有toBeComplete为NONE且tokensOffset==0时，才能开始下一个Action
    //begin之后会一直行动，直到完成，然后更新。

    //headOperator必须满足要求：如果是JUMP，则操作数times为1；如果是READ，则操作数为1.
    bool beginAction(HeadOperator headOperator){
        if(toBeComplete.action!=NONE){
            return false;
        }
        if(tokensOffset > 0){
            return false;
        }
        LOG_ACTIONSN(diskId) << "begin action" << headOperator;
        if((headOperator.action == READ||headOperator.action == VREAD) && presentTokens > 0){
            toBeComplete = headOperator;
            return true;
        }else if(headOperator.action == PASS && presentTokens > 0){
            toBeComplete = headOperator;
            return true;
        }else if(headOperator.action == JUMP && presentTokens == G
                     && headOperator.jumpTo >=0 && headOperator.jumpTo < spaceSize){//只有在时间步开始才能进行JUMP
            toBeComplete = headOperator;
            return true;
        }
        return false;
    }
    //先假设读操作能跨多次。
    
    /// @brief complete action began before
    /// @param handledOperation 把开始的行动都push_back到该参数中。（便于得到输出到判题器的参数）
    /// @param completedRead 把完成的读都push_back到该参数中。（便于判断哪些单元已经被读完了）
    /// @return true if complete. false when has no enough tokens to complete.
    bool completeAction(std::vector<HeadOperator>* handledOperation, std::vector<int>* completedRead){
        LOG_ACTIONSN(diskId) << "try complete action" << toBeComplete;
        auto action = toBeComplete.action;
        if(toBeComplete.action == NONE){
            return true;
        }
        if(action == READ|| action == VREAD) {
            if(tokensOffset>0 && presentTokens >= tokensOffset){
                //当前策略不存在这种情况
                assert(false);
                //如果已经有读开始，但是还未结束
                if (completedRead != nullptr && action != VREAD) {
                    (*completedRead).push_back(headPos);
                }

                headPos += 1; headPos %= spaceSize;

                presentTokens -= tokensOffset;
                tokensOffset = 0;
                
                readConsume = getNextReadConsume(readConsume);
            }else if(readConsume <= presentTokens){
                //如果没有读开始，刚开始且能够完成
                if (completedRead != nullptr && action != VREAD) {
                    (*completedRead).push_back(headPos);
                }

                headPos += 1; headPos %= spaceSize;

                presentTokens -= readConsume;
                readConsume = getNextReadConsume(readConsume);
            }else{//无法完成当前读。
                return false;//无法跨两步读。
                // //如果没有读开始，但是无法完成
                // if(i>0){
                //     handledOperation->push_back({READ, i});//返回已经开始或完成的操作。
                // }
                // tokensOffset = readConsume - presentTokens;
                // presentTokens = 0;
                // return false;
            }
            handledOperation->push_back(toBeComplete);//完成该行动。
            toBeComplete = {NONE, 0};
            return true;
        }else if(action == PASS){
            if(presentTokens == 0){
                return false;
            }
            if(toBeComplete.passTimes <= presentTokens){
                headPos += toBeComplete.passTimes; headPos %= spaceSize;
                handledOperation->push_back({toBeComplete.action, toBeComplete.passTimes});

                presentTokens -= toBeComplete.passTimes;
                toBeComplete = {NONE, 0};
                readConsume = FIRST_READ_CONSUME;
                return true;
            }else{
                headPos += presentTokens; headPos %= spaceSize;
                handledOperation->push_back({PASS, presentTokens});

                toBeComplete.passTimes -= presentTokens;
                presentTokens = 0;
                readConsume = FIRST_READ_CONSUME;
                return false;
            }
        }else if(action == JUMP){
            auto jumpTo = toBeComplete.jumpTo;
            if(presentTokens == G && tokensOffset==0){
                handledOperation->push_back({JUMP, jumpTo});

                headPos = jumpTo;

                presentTokens = 0;
                readConsume = FIRST_READ_CONSUME;

                toBeComplete = {NONE, 0};
                return true;
            }else{
                return false;
            }
        }
        return false;
    };
    //把取消的Read提取出来。其它操作没有必要提取。READ会对ReqUnit造成影响。
    void cancelAction(std::vector<int>* canceledRead) {
        if (this->tokensOffset) {
            this->presentTokens = this->presentTokens - this->tokensOffset;
        }
        int pos = this->headPos;
        if (this->toBeComplete.action == READ || this->toBeComplete.action == VREAD) {
            canceledRead->push_back(this->headPos);
        }
        this->toBeComplete = { NONE, 0 };
    }
    // 计算令牌花费
    // @param action 进行的操作。
    // @param times 操作的次数。
    // @return 返回令牌花费数。
};

struct DiskUnit{
    int objId = 0;
    int untId = 0;
    
};

class Disk:noncopyable{
public:
    std::map<int, std::pair<SpacePieceBlock*, StorageMode>> freeSpace;
    std::vector<std::pair<int, SpacePieceBlock*>> endToFreeSpace;
    #ifdef ENABLE_SCATTEROBJS
    std::vector<Object*> scatterObjs;
    #endif
private:
    //不仅存储对象id(int)，还存储unit值（char*),字节对齐会增加额外空间。
    //也可以用两个space，一个存char，一个存int。
    //CircularSpacePiece freeSpace;
    Dim1Space<DiskUnit> diskSpace;
    
    int bestHeadPos;

    //记录磁盘内各个tag占据的unit数量。
    int* tagUnitNum;//在assign这种存储对象粒度的方法中修改。而非store这种单元粒度的方法中。
    
    /// <summary>
    /// 
    /// </summary>
    /// <param name="objId"></param>
    /// <param name="unitOnDisk"></param>
    /// <param name="it">对象的单元排列的顺序</param>
    /// <param name="start"></param>
    /// <param name="len"></param>
    void allocate(SpacePieceBlock* space, StorageMode mode, int objId, int* unitOnDisk, std::vector<int>::iterator it, int start, int len){
        //LOG_LINKEDSPACEN(diskId) << "alloc space for obj "<<objId<<
        //    " on " <<this->diskId<<" (start:"<<start<<" len:"<<len<<")";
        for(int i=0;i<len;i++){
            int pos = (start+i)%spaceSize;
            diskSpace[pos].objId = objId;
            diskSpace[pos].untId = *it;
            unitOnDisk[*it] = pos;
            LOG_DISKN(diskId)<<"unit "<<*it << " on "<<pos;
            it++;
        }
        space->alloc(start, len, mode);
        //LOG_LINKEDSPACEN(diskId) << space;
    }
    //默认每个对象只会被分配到同一块SpacePieceBlock
    void dealloc(int* unitOnDisk, int objSize, int tag, StorageMode mode){
        if(objSize == 1){
            if (freeSpace[tag].first->inSpace(unitOnDisk[0])) {
                freeSpace[tag].first->deAlloc(unitOnDisk[0], 1, mode);
            }
            else {
                for (int i=0; i < endToFreeSpace.size(); i++) {
                    if (unitOnDisk[0] <= endToFreeSpace[i].first) {
                        endToFreeSpace[i].second->deAlloc(unitOnDisk[0], 1, mode);
                        break;
                    }
                }
            }
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
                // LOG_LINKEDSPACEN(diskId) << "free unit on position " << start<<" with len "<<len;
                if (freeSpace[tag].first->inSpace(unitOnDisk[0])) {
                    freeSpace[tag].first->deAlloc(start, len, mode);
                }
                else {
                    for (int j=0; j < endToFreeSpace.size(); j++) {
                        if (unitOnDisk[0] <= endToFreeSpace[j].first) {
                            endToFreeSpace[j].second->deAlloc(start, len, mode);
                            break;
                        }
                    }
                }
                
                diskSpace[start].objId = 0;//0是被删除的object
                //LOG_LINKEDSPACEN(diskId) << "free over";
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
    const int diskId;
    const int spaceSize;
    std::array<DiskHead*, HEAD_NUM> heads;
    //存放被请求的单元信息。
    Disk(int id, int spacesize, std::vector<std::pair<int, int>> tagToSpaceSize)
        :diskId(id), spaceSize(spacesize){
        for(int i=0;i<HEAD_NUM;i++){
            this->heads[i] = new DiskHead(i, spaceSize);
            this->heads[i]->diskId = id;
        }
        //LOG_DISK << "start init space";
        diskSpace.initSpace(spacesize);
        for(int i=0;i<spacesize;i++){
            diskSpace[i].objId = -200;
        }

        //LOG_DISK << "malloc space for tagUnitNum";
        this->tagUnitNum = (int*)malloc(sizeof(int)*M);
        for(int i=0;i<M;i++){
            this->tagUnitNum[i] = 0;
        }
        //LOG_DISK << "malloc space for tagUnitNum over";

        //free space linked list
        int start = 0; int len = 0;
        assert(tagToSpaceSize.size() % 2 == 0);//无法处理tag数为奇数情况。待处理。
        for (int i = 0; i < tagToSpaceSize.size(); i++) {
            len += tagToSpaceSize[i].second;
            if (i % 2 == 1) {
                auto newSpace = new SpacePieceBlock(start, len, {tagToSpaceSize[i - 1].first, tagToSpaceSize[i].first});
                freeSpace.insert({ tagToSpaceSize[i - 1].first,  {newSpace, StoreFromFront} });
                freeSpace.insert({ tagToSpaceSize[i].first,  {newSpace, StoreFromEnd} });
                start += len;
                endToFreeSpace.push_back({ start - 1 , newSpace });//start+len-1即当前的end
                len = 0;
            }
        }
    }
    
    /// @brief 通过存储单元位置返回存储单元信息
    /// @param unitPos 存储单元的位置
    /// @return 返回存储单元的信息，包括存储单元所属的对象Id及其在对象中的位置Id
    const DiskUnit& getUnitInfo(int unitPos) const {
        // if(unitPos<0||unitPos>=diskSpace.getBufLen()){
        //     //LOG_DISK << "index " << unitPos <<" out of range!";
        //     throw std::out_of_range("buffer space out of range!");
        // }
        return diskSpace[unitPos];
    }
    //返回从from位置移动到target需要经过的单元距离。
    int getDistance(int target, int from){
        return (target - from + spaceSize)%spaceSize;
    }

    //freeSpace相关
    //urgent表明该对象写入时即需要读取。tag表明是否按tag尽量分在同一tag周围（或相关性较大tag周围）。
    void assignSpace(Object& obj ,int repId ,int tag){
        //LOG_LINKEDSPACEN(this->diskId) << "\nstart assign space for obj " << obj << " of tag " << tag;
        int objSize = obj.size;
        int* unitOnDisk = obj.unitOnDisk[repId];
        //std::vector<int> units = formUnitsByOrder(objSize, static_cast<UnitOrder>(repId));
        std::vector<int> units = {};
        for(int i=0;i<objSize;i++){
            units.push_back(i);
        }
        //尝试寻找一块合适大小空间。
        auto mode = freeSpace[tag].second;
        auto space = freeSpace[tag].first;
        auto nodes = space->getNodes();
        std::list<SpacePiece>::iterator it;
        if (mode == StoreFromFront) {
            it = nodes->begin();
            for (int i = 0; i < 20; i++) {
                if (it != nodes->end() && (*it).len < objSize) {
                    it++;
                }
                else { break; }
            }

            if (it != nodes->end() && (*it).len >= objSize) {//如果存在能完整放入该对象的空间
                //LOG_LINKEDSPACEN(diskId) << "find an whole space, spaceSize, start " << (*it).start << " len " << (*it).len <<
                //    "for space: " << objSize;
                int start = (*it).start;
                allocate(space, mode, obj.objId, unitOnDisk, units.begin(), start, objSize);
            }
            else {//找不到一整块足够大小的空间。直接分散存储（效率会较低）。
                //FIXME:应该记录最大空间大小！或者分级存储freeSpace。
                #ifdef ENABLE_SCATTEROBJS
                obj.scatterStore[repId] = true;
                scatterObjs.push_back(&obj);
                #endif
                int leftSize = objSize;
                int tempAlloc = 0;
                //LOG_LINKEDSPACEN(diskId) << "can't find continuous space. tolSize:" << objSize;
                while (leftSize > 0) {
                    it = nodes->begin();
                    //LOG_LINKEDSPACEN(diskId) << "alloc start " << (*it).start << " part len:" << (*it).len ;
                    int allocNum = std::min<int>(leftSize, (*it).len);
                    leftSize -= (*it).len;
                    allocate(space, mode, obj.objId, unitOnDisk, units.begin()+tempAlloc, 
                            (*it).start, allocNum);
                    tempAlloc = objSize - leftSize;
                    
                }
            }
        }
        else {
            it = nodes->end(); it--;
            for (int i = 0; i < 20; i++) {
                if (it != nodes->begin() && (*it).len < objSize) {
                    it--;
                }
                else { break; }
            }

            if ((*it).len >= objSize) {//如果存在能完整放入该对象的空间
                //LOG_LINKEDSPACEN(diskId) << "find an whole space, spaceSize, start " << (*it).start << " len " << (*it).len <<
                //    "for space: " << objSize <<" at space end";
                int start = (*it).start + (*it).len - objSize;
                allocate(space, mode, obj.objId, unitOnDisk, units.begin(), start, objSize);
            }
            else {//FIXME:应该记录最大空间大小?或者分级存储freeSpace。
                #ifdef ENABLE_SCATTEROBJS
                obj.scatterStore[repId] = true;
                scatterObjs.push_back(&obj);
                #endif
                int leftSize = objSize;
                int tempAlloc = 0;
                //LOG_LINKEDSPACEN(diskId) << "can't find continuous space. tolSize:" << objSize;
                while (leftSize > 0) {//默认空间足够，否则可能转一圈。
                    it = nodes->end(); it--;
                    int allocNum = std::min<int>(leftSize, (*it).len);
                    //LOG_LINKEDSPACEN(diskId) << "alloc start " << (*it).start << " part len:" << (*it).len;
                    leftSize -= (*it).len;
                    allocate(space, mode, obj.objId, unitOnDisk, units.begin() + tempAlloc,
                        (*it).start + (*it).len - allocNum, allocNum);
                    tempAlloc = objSize - leftSize;
                    
                }
            }
        }
        //LOG_LINKEDSPACEN(this->diskId) << "over assign space for obj " << obj;
        //LOG_LINKEDSPACEN(this->diskId) <<"assign over" << *space;
        //虽然头数据知道对象数量，但不知道对象大小。
        //可以规划分区。
        //可以通过小size对象来减小磁盘碎片。
        //在空的地方读会报错吗？
        this->tagUnitNum[tag] += objSize;
    }
    void releaseSpace(Object& obj ,int repId){
        #ifdef ENABLE_SCATTEROBJS
        if(obj.scatterStore[repId]){
            auto it = std::find(scatterObjs.begin(), scatterObjs.end(), &obj);
            if(it == scatterObjs.end()){
                throw std::logic_error("can't find scatter stored obj");
            }
            scatterObjs.erase(it);
        }
        #endif
        this->dealloc(obj.unitOnDisk[repId], obj.size, obj.tag, freeSpace[obj.tag].second);
        this->tagUnitNum[obj.tag] -= obj.size;
    }

    int getFreeSpaceSize(int tag){return freeSpace[tag].first->getResidualSize();}
    //优化方向：严格来说，应该以之后还会来多少req以及req的分散/集中性为指标
    int calReqUnitNum() {
        int reqUnitNum = 0;
        for (int i = 0; i < M; i++) {
            reqUnitNum += tagUnitNum[i] * StaBkt::getPresentReqLeft(i)/100;
        }
        return reqUnitNum;
    }
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



#endif