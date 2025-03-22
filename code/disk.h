#ifndef DISK_H
#define DISK_H
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <cmath>

#include "noncopyable.h"
#include "bufferSpace.h"
#include "circularLinkedList.h"

#include "global.h"
#include "object.h"
#include "bplusTree.h"
enum HeadAction{
    NONE,
    JUMP,
    PASS,
    READ
};

struct HeadOperator{
    HeadAction action;
    union{
        int times;
        int jumpTo;
    };
};

enum UnitOrder{
    SEQUENCE,//顺序，01234
    RESERVE,//倒序， 43210
    INSERT//插序， 24130(start=size/2, interval=7(选一个大于最大size的最小质数))
};

#define INTERVAL 7
#define LOG_DISK LOG_FILE("disk")

class DiskHead{
private:
    
public:
    const int spaceSize;
    int headPos = 0;
    int readConsume = FIRST_READ_CONSUME;
    int presentTokens = G;
    int tokensOffset = 0;//在上个回合花的下个回合的令牌。
    HeadOperator toBeComplete = {NONE, 0};
    DiskHead(int spacesize):spaceSize(spacesize){}
    void freshTokens(){
        this->presentTokens = G - this->tokensOffset;
        this->tokensOffset = 0;
    }
    int calTokensCost(HeadOperator headOperator){
        auto action = headOperator.action;
        auto times = headOperator.times;
        if(action == READ){
            int tolConsume = 0;
            for(int i=0;i<times;i++){
                int tempReadConsum = readConsume;
                tolConsume += tempReadConsum;
                tempReadConsum = std::max<int>(16, std::ceil(0.8*readConsume));
            }
            return tolConsume;
        }else if(action == PASS){
            return times;
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
        if(headOperator.action == READ && presentTokens > 0){
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
        auto action = toBeComplete.action;
        if(toBeComplete.action == NONE){
            return true;
        }
        if(action == READ){
            for(int i=0;i<toBeComplete.times;i++){
                if(tokensOffset>0 && presentTokens >= tokensOffset){
                    //如果已经有读开始，但是还未结束
                    (*completedRead).push_back(headPos);

                    headPos += 1; headPos %= spaceSize;

                    presentTokens -= tokensOffset;
                    tokensOffset = 0;
                    toBeComplete.times -= 1;
                    
                    readConsume = std::max<int>(16, std::ceil(0.8*readConsume));
                }else if(readConsume <= presentTokens){
                    //如果没有读开始，刚开始且能够完成
                    (*completedRead).push_back(headPos);

                    headPos += 1; headPos %= spaceSize;

                    presentTokens -= readConsume;
                    toBeComplete.times -= 1;
                    readConsume = std::max<int>(16, std::ceil(0.8*readConsume));
                }else{
                    //如果没有读开始，但是无法完成
                    if(i>0){
                        handledOperation->push_back({READ, i});//返回已经开始或完成的操作。
                    }
                    tokensOffset = readConsume - presentTokens;
                    presentTokens = 0;
                    return false;
                }
            }
            handledOperation->push_back({toBeComplete.action, toBeComplete.times});
            toBeComplete = {NONE, 0};
        }else if(action == PASS){
            if(presentTokens == 0){
                return false;
            }
            if(toBeComplete.times <= presentTokens){
                headPos += toBeComplete.times; headPos %= spaceSize;
                handledOperation->push_back({toBeComplete.action, toBeComplete.times});

                presentTokens -= toBeComplete.times;
                toBeComplete = {NONE, 0};
                readConsume = FIRST_READ_CONSUME;
                return true;
            }else{
                headPos += presentTokens; headPos %= spaceSize;
                handledOperation->push_back({PASS, presentTokens});

                toBeComplete.times -= presentTokens;
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
            }else{
                return false;
            }
        }
        return false;
    };
    
    // 计算令牌花费
    // @param action 进行的操作。
    // @param times 操作的次数。
    // @return 返回令牌花费数。
};

struct DiskUnit{
    int objId;
    int untId;
};
class Disk:noncopyable{
private:
    //不仅存储对象id(int)，还存储unit值（char*),字节对齐会增加额外空间。
    //也可以用两个space，一个存char，一个存int。
    CircularSpacePiece freeSpace;
    Dim1Space<DiskUnit> diskSpace;

    int bestHeadPos;

    //记录磁盘内各个tag占据的unit数量。
    int* tagUnitNum;//在assign这种存储对象粒度的方法中修改。而非store这种单元粒度的方法中。
    
    void allocate(int objId, int* unitOnDisk, std::vector<int>::iterator it, int start, int len){
        for(int i=0;i<len;i++){
            int pos = (start+len)%spaceSize;
            diskSpace[pos].objId = objId;
            diskSpace[pos].untId = *it;
            // objIdSpace[pos] = objId;
            // untIdSpace[pos] = *it;
            unitOnDisk[*it] = pos;
            it++;
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
    const int diskId;
    const int spaceSize;
    DiskHead head;
    //存放被请求的单元信息。
    Disk(int id, int spacesize, int headPos = 0)
        :freeSpace(spacesize), diskId(id), head(spacesize), spaceSize(spacesize){
        this->head.headPos = headPos;
        diskSpace.initSpace(spacesize);

        tagUnitNum = (int*)malloc(sizeof(int)*M);
        memcpy(tagUnitNum, 0, sizeof(int)*M);//初始化。
    }
    
    /// @brief 通过存储单元位置返回存储单元信息
    /// @param unitPos 存储单元的位置
    /// @return 返回存储单元的信息，包括存储单元所属的对象Id及其在对象中的位置Id
    DiskUnit getUnitInfo(int unitPos){return diskSpace[unitPos];}
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
    
    //返回从from位置移动到target需要经过的单元距离。
    int getDistance(int target, int from){
        return (target - from + spaceSize)%spaceSize;
    }

    //freeSpace相关
    //urgent表明该对象写入时即需要读取。tag表明是否按tag尽量分在同一tag周围（或相关性较大tag周围）。
    void assignSpace(Object& obj ,UnitOrder order ,int* unitOnDisk ,bool urgent, int tag = -1){
        int objSize = obj.size;
        std::vector<int> units = formUnitsByOrder(objSize, order);////////////////////////
        const SpacePieceNode* node = freeSpace.getStartAfter(head.headPos, true);
        auto tempNode = node;
        if(node->getLen() < objSize){//找到一块合适大小的空间
            tempNode = node->getNext();
            while(tempNode != node && tempNode->getLen() < objSize){
                tempNode = tempNode->getNext();
            }
        }

        if(tempNode->getLen() >= objSize){//如果存在能完整放入该对象的空间
            int start = node->getStart();
            if(start==head.headPos){//即如果当前磁头位置处于空白区域的起始处，也即磁头位置没有存储内容。
                int end = node->getLen()+start;
                allocate(obj.objId, unitOnDisk, units.begin(), end-objSize, objSize);//放在这块空间的尾部
            }else{
                allocate(obj.objId, unitOnDisk, units.begin(), start, objSize);//放在这块空间的首部（与head靠近）
            }
        }else{//找不到一整块足够大小的空间。直接分散存储（效率会较低）。
            //FIXME:应该记录最大空间大小！或者分级存储freeSpace。
            int leftSize = objSize;
            while(leftSize>0){
                allocate(obj.objId, unitOnDisk, units.begin()+objSize-leftSize, tempNode->getStart(), std::min<int>(tempNode->getLen(), leftSize));
                leftSize = leftSize - tempNode->getLen();
            }
        }
        //虽然头数据知道对象数量，但不知道对象大小。
        //可以规划分区。
        //可以通过小size对象来减小磁盘碎片。
        //在空的地方读会报错吗？
        this->tagUnitNum[tag] += objSize;
    }
    void releaseSpace(int* unitOnDisk, int objSize, int tag){
        this->dealloc(unitOnDisk, objSize);
        this->tagUnitNum[tag] -= objSize;
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

class HeadActionsInfo{
public:
    int spaceSize = 1;
    int tokenCost = 0;
    int orgHeadPos = 0;
    int diskId = -1;
    int nextReadConsume = FIRST_READ_CONSUME;
    std::vector<HeadOperator> actions = {};
    std::vector<int> headPoses = {};
    // HeadActionsInfo(int readConsume, int headpos, int spacesize):
    //     nextReadConsume(readConsume), orgHeadPos(headpos), spaceSize(spacesize){}
    HeadActionsInfo(Disk* disk):
        nextReadConsume(disk->head.readConsume), 
        orgHeadPos(disk->head.headPos), 
        spaceSize(disk->head.spaceSize),
        tokenCost(G - disk->head.presentTokens),//该回合已经花费的tokens
        diskId(disk->diskId)
    {if(disk->head.tokensOffset==0){
            throw std::logic_error("don't initialize headActionsInfo by head with action incompleted.");
    }}//不要用还未完成行动的head来初始化HeadActionsInfo！
    // 将操作序列附加到现有操作操作序列中，并更新所有操作的累积花费和读操作状态。
    // @param actions 要附加的操作序列。
    int getLastHeadPos(){
        if(headPoses.size() == 0){
            return orgHeadPos;
        }else{
            return headPoses[headPoses.size()-1];
        }
    }
    void freshActionsInfo(std::vector<HeadOperator> actions){
        for(int i=0;i<actions.size();i++){
            auto operation = actions[i];
            freshActionsInfo(operation);
        }
    }
    void freshActionsInfo(HeadOperator operation){
        auto action = operation.action;
        if(action == READ){
            auto times = operation.times;
            for(int i=0;i<times;i++){
                this->tokenCost += this->nextReadConsume;
                this->nextReadConsume = std::max<int>(16, std::ceil(0.8*this->nextReadConsume));
            }
            freshHeadPosesMove(times);
        }else if(action == PASS){
            auto times = operation.times;
            this->tokenCost += times;
            this->nextReadConsume = FIRST_READ_CONSUME;
            freshHeadPosesMove(times);
        }else if(action == JUMP){
            auto jumpTo = operation.jumpTo;
            if(jumpTo<0 || jumpTo>=spaceSize){
                throw std::out_of_range("action JUMP jumps out of range");
            }
            if(this->tokenCost%G>0){
                this->tokenCost = (this->tokenCost/G+1)*G+G;
            }else{
                this->tokenCost += G;
            }
            this->nextReadConsume = FIRST_READ_CONSUME;
            freshHeadPosesJump(jumpTo);
        }
        this->actions.push_back(operation);
    }
    void freshHeadPosesMove(int offset){
        int actionNum = headPoses.size();
        if(actionNum){headPoses.push_back((headPoses[actionNum-1]+offset)%spaceSize);}
        else{headPoses.push_back((orgHeadPos+offset)%spaceSize);}
    }
    void freshHeadPosesJump(int target){
        headPoses.push_back(target);
    }
    // int getTokenCost(){return tokenCost;}
    // int getTimeCost(){return timeCost;}
    // int getNextReadConsume(){return nextReadConsume;}
    // std::vector<std::pair<HeadAction, int>> getActions(){return actions;}
};
/// @brief 负责：从请求单元链表中删除已读取的请求单元
///
///请求的信息包括：diskInfo中的reqSpace和actionsPlan、object中的planUnit、
class DiskManager{
public:
    struct DiskInfo{
        Disk* disk = nullptr;
        BplusTree<6> reqSpace;
        std::vector<HeadOperator> handledOperations;//作为输出到判题器的参数。
        std::vector<int> completedRead;
        //行为策略。
        void toReqUnit(HeadActionsInfo& actionsInfo, int reqUnit){
            int distance = disk->getDistance(reqUnit, actionsInfo.getLastHeadPos());//choseRep已经算过了一遍，这里再算一遍？！
            if(distance){
                if(distance <= disk->head.presentTokens){
                    //如果可以直接移动到目标位置。
                    actionsInfo.freshActionsInfo(HeadOperator{PASS, distance});
                    //也可以试试能不能连读过去（节省时间），这取决于下面有多少单元，能否节省总开销。
                }else if(disk->head.presentTokens == G){
                    //当前还有G个Token，即上一时间步的操作没有延续到当前时间步。可以直接跳过去。
                    actionsInfo.freshActionsInfo(HeadOperator{JUMP, reqUnit});
                }else if(disk->head.presentTokens + G > distance){
                    //如果当前时间步的令牌数不够跳，且这个时间步+下个时间步能够移动到目标位置，
                    //那么先向目标位置移动
                    actionsInfo.freshActionsInfo(HeadOperator{PASS, disk->head.presentTokens});
                }else{
                    //最早也得下两个回合才能到达读位置。
                    //可以视情况做一些有益的策略。（如移动到下回合更可能有请求的位置。
                    //也可以不行动。
                }
            }
        }
        //判断是否能够连读。如果能，则连读。
        void multiRead(HeadActionsInfo& actionsInfo){
            int times = 0;
            int reqUnit = actionsInfo.getLastHeadPos();
            DiskUnit unitInfo = this->disk->getUnitInfo(reqUnit);
            //如果reqUnit还未被规划，则规划，且查看是否可以连读。
            while(sObjectsPtr[unitInfo.objId]->unitReqNum[unitInfo.untId] > 0 && 
                    (!sObjectsPtr[unitInfo.objId]->isPlaned(unitInfo.untId))){
                sObjectsPtr[unitInfo.objId]->plan(unitInfo.untId, this->disk->diskId);
                reqUnit ++; times ++;
                unitInfo = this->disk->getUnitInfo(reqUnit);
            }
            actionsInfo.freshActionsInfo({READ, times});
        }

        //reqUnit相关
        bool roundReqUnitToPos(int headPos){
            if(this->reqSpace.getKeyNum()>0){
                this->reqSpace.setAnchor(headPos);
                return true;
            }else{
                return false;
            }
            
        }
        
        /// @brief 获取下一个key
        /// @param toNext 是否把锚点移动到下一个key处。
        /// @return 如果已经没有下一个请求单元，则返回-1，否则返回请求单元的位置
        int getNextRequestUnit(bool toNext = true){
            // try{
            return this->reqSpace.getNextKeyByAnchor(toNext);
            // }catch(const std::logic_error& e){
            //     throw;
            // }
        }

        /// @brief 获取下一个未被规划的请求单元，并且把锚点移动到该请求单元处。
        /// @return 如果已经没有下一个未被规划的请求单元，则返回-1，否则返回请求单元的位置
        int getNextUnplanedReqUnit(){
            int reqUnit = this->reqSpace.getNextKeyByAnchor(true);
            if(reqUnit == -1){
                return reqUnit;
            }
            auto diskUnit = disk->getUnitInfo(reqUnit);
            while(sObjectsPtr[diskUnit.objId]->isPlaned(diskUnit.untId) 
                && reqUnit != -1){
                reqUnit = this->reqSpace.getNextKeyByAnchor(true);
                diskUnit = disk->getUnitInfo(reqUnit);
            }//直到找到一个还未被规划的unit

            return reqUnit;
        }

        void addReqUnit(int unitPos){
            this->reqSpace.insert(unitPos);
        }
        void rmReqUnit(int unitPos){
            this->reqSpace.remove(unitPos);
        }
        bool hasRequestUnit(){
            if(this->reqSpace.root->keyNum>0){
                return true;
            }
            return false;
        }
    };
    std::vector<DiskInfo*> diskGroup;
    std::vector<int> doneRequestIds;
public:
    //存储一个活动对象的id索引。方便跟进需要查找的单元信息。
    DiskManager(){};
    void addDisk(int spaceSize){
        diskGroup.push_back(new DiskInfo{new Disk(diskGroup.size(), spaceSize)});
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
                    diskGroup[diskId]->reqSpace.insert(obj.unitOnDisk[i][u]);
                }
            }
        }
    }
    void freshOvertimeReqUnits(const Object& obj, std::vector<int> unitsOrder){
        for(int r=0;r<REP_NUM;r++){//第几个副本
            DiskInfo* disk = this->diskGroup[obj.replica[r]];
            for(int i=0;i<unitsOrder.size();i++){
                disk->reqSpace.remove(obj.unitOnDisk[r][unitsOrder[i]]);
            }
        }
    }

    void assignSpace(Object& obj){
        //首先挑出3块空间够且负载低的磁盘，然后选择存储位置，然后选择存储顺序。原则如下：
        /*
            存储位置：同一对象分配的空间尽量连续（方便连读）。
            存储顺序：三个副本尽量采用不同的存储顺序。
        */
        int* diskSort = (int*)malloc(sizeof(int)*diskGroup.size());
        for(int i=0;i<diskGroup.size();i++){
            diskSort[i] = i;
        }
        std::sort<int*>(diskSort, diskSort+diskGroup.size(), [=](int a, int b){
            return (diskGroup[a]->disk->getFreeSpaceSize() > diskGroup[b]->disk->getFreeSpaceSize());
        });

        for(int i=0;i<REP_NUM;i++){
            diskGroup[diskSort[i]]->disk->assignSpace(obj, static_cast<UnitOrder>(i), obj.unitOnDisk[i], false, obj.tag);
            obj.replica[i] = diskSort[i];
        }//分配空间最大的磁盘。

        free(diskSort);
        
    }

    /// @brief chose a disk to read unit reqUnit
    /// @param reqUnit unit position in disk
    /// @return the disk get this unit
    int choseRep(int objId, int untId){
        //离谁进就选谁。还有一个简单策略是谁目前的时间片规划得少就选谁。
        //但是这需要考虑到在磁头路径中间插入请求单元
        //因此要在actionsInfo中记录所有计划完成的请求单元（其实headpos已经记录了）
        //并且需要从头遍历路径来选择一个合适的位置插入这个请求单元
        int minDistDiskId = -1;
        int dist = -1;

        Object* obj = sObjectsPtr[objId];
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj->replica[i];
            int untPos = obj->unitOnDisk[i][untId];
            int headpos = diskGroup[diskId]->disk->head.headPos;
            int tempDisk = diskGroup[diskId]->disk->getDistance(untPos, headpos);
            
            if(dist<0){
                dist = tempDisk;
                minDistDiskId = diskId;
            }else if(dist>tempDisk){
                dist = tempDisk;
                minDistDiskId = diskId;
            }
        }

        return minDistDiskId;
    }
    //choseReq如果每次只规划一个单元，那么就没有依据给连读判断。
    void planUnitsRead(){
        std::map<int, HeadActionsInfo*> vHeadsMap;
        std::vector<HeadActionsInfo*> vHeads;
        DiskInfo* diskInfo;
        Disk* disk;
        for(int i=0;i<this->diskGroup.size();i++){
            diskInfo = this->diskGroup[i];
            disk = diskInfo->disk;
            if(disk->head.completeAction(&diskInfo->handledOperations, &diskInfo->completedRead)){
                if(diskInfo->roundReqUnitToPos(disk->head.headPos)){
                    vHeadsMap.insert({disk->diskId, new HeadActionsInfo(disk)});
                    vHeads.push_back(vHeadsMap[disk->diskId]);
                };
            }
        }
        auto lambdaCompare = [](HeadActionsInfo*& a, HeadActionsInfo*& b){
                return a->tokenCost<b->tokenCost;
        };
        
        std::sort(vHeads.begin(), vHeads.end(), lambdaCompare);
        while(vHeads.size() && vHeads[0]->tokenCost<G){//规划一个回合。
            auto vHead = vHeads[0];//选择花费的cost最少的那个头；
            diskInfo = this->diskGroup[vHead->diskId];
            disk = diskInfo->disk;

            int unitPos = diskInfo->getNextUnplanedReqUnit();
            if(unitPos>=0){
                auto unitInfo = disk->getUnitInfo(unitPos);
                diskInfo->toReqUnit(*vHead, unitPos);
                diskInfo->multiRead(*vHead);

                //如果规划完成则将其删除。
                if(vHead->tokenCost>=G){
                    commitPlan(vHead->diskId, vHead->actions);
                    vHeads.erase(vHeads.begin());//不会改变顺序
                }
            }else{//如果请求已经转完一圈了就停止。
                commitPlan(vHead->diskId, vHead->actions);
                vHeads.erase(vHeads.begin());//不会改变顺序
            }
            std::sort(vHeads.begin(), vHeads.end(), lambdaCompare);
        }
    }

    //在请求单元链表中删除obj的所有第unitOrder个unit的副本。
    void removeObjectReqUnit(const Object& obj, int unitOrder){
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj.replica[i];
            int unitPos = obj.unitOnDisk[i][unitOrder];
            diskGroup[diskId]->reqSpace.remove(unitPos);
        }
    }
    //执行diskId的已有规划
    //将实际开始执行的行动记录到diskInfo->handledOperations中。
    //将实际执行完成的请求记录到doneRequestIds中。
    void commitPlan(int diskId, std::vector<HeadOperator> actionsPlan){
        DiskInfo* diskInfo = this->diskGroup[diskId];
        Disk* disk = diskInfo->disk;
        //首先completeAction，完成上一次没有完成的持续行动。
        //如果行动在这一回合还是没有完成，那么就接收两个参数。
        //应当在之前先对所有disk试试completeAction。如果返回false，那么此时的presentTokens肯定为0
        if(disk->head.completeAction(&diskInfo->handledOperations, &diskInfo->completedRead)){
            int planNum = actionsPlan.size();
            for(int i=0;i<planNum;i++){
                auto action = actionsPlan[0];
                if(!disk->head.beginAction(action)){
                    break;//要么是之前行动没有做完，要么是输入参数出了问题。
                }else{
                    actionsPlan.erase(actionsPlan.begin());//删除喂入的行动
                    if(!disk->head.completeAction(&diskInfo->handledOperations, &diskInfo->completedRead)){
                        break;//剩下的时间片不足以完成这个行动
                    }
                    //剩下的时间片完成了这个行动。继续迭代以喂入下一个活动。
                }
            }
        }
    }
    void freshDoneRequestIds(Disk* disk, std::vector<int> completedRead){
        for(int i=0;i<completedRead.size();i++){
            int readUnit = completedRead[i];
            auto unitInfo = disk->getUnitInfo(readUnit);
            Object* obj = sObjectsPtr[unitInfo.objId];
            //如果这回合刚读完之前的一个行动，但是这回合对应的对象被删除了，那就无效了。
            if(obj != &deletedObject){
                obj->commitUnit(unitInfo.untId, &doneRequestIds);
                this->removeObjectReqUnit(*sObjectsPtr[unitInfo.objId], unitInfo.untId);
            }
        }
    }
    //返回该回合对应磁盘磁头执行的所有动作
    std::vector<HeadOperator> getHandledOperations(int diskId){
        return std::move(diskGroup[diskId]->handledOperations);
    }
    //返回该回合完成的所有请求
    std::vector<int> getDoneRequests(){
        for(int i=0;i<diskGroup.size();i++){
            freshDoneRequestIds(diskGroup[i]->disk, std::move(diskGroup[i]->completedRead));
        }
        return std::move(doneRequestIds);
    }

    void freeSpace(Object& obj){
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj.replica[i];
            Disk* disk = diskGroup[diskId]->disk;
            disk->releaseSpace(obj.unitOnDisk[i], obj.size, obj.tag);
        }
    }
    
    void freshTokens(){
        for(int i=0;i<diskGroup.size();i++){
            diskGroup[i]->disk->head.freshTokens();
        }
    }

};

#endif