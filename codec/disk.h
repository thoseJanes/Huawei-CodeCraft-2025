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

#define LOG_DISK LOG_FILE("disk")
#define LOG_ACTIONS LOG_FILE("actions")
#define LOG_DISKN(x) LOG_FILE("disk"+std::to_string(x))
#define LOG_ACTIONSN(x) LOG_FILE("actions"+std::to_string(x))
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
LogStream& operator<<(LogStream& s, HeadOperator& headOperator) {
    if (headOperator.action == JUMP) {
        s << "[JUMP " << headOperator.jumpTo << "]";
    }
    else if (headOperator.action == PASS) {
        s << "[PASS " << headOperator.times << "]";
    }
    else if (headOperator.action == READ) {
        s << "[READ " << headOperator.times << "]";
    }
    return s;
}

enum UnitOrder{
    SEQUENCE,//顺序，01234
    RESERVE,//倒序， 43210
    INSERT//插序， 24130(start=size/2, interval=7(选一个大于最大size的最小质数))
};

#define INTERVAL 7

class DiskHead{
private:
    
public:
    int id;
    const int spaceSize;
    int headPos = 0;
    int readConsume = FIRST_READ_CONSUME;
    int presentTokens = G;
    int tokensOffset = 0;//在上个回合花的下个回合的令牌。
    HeadOperator toBeComplete = {NONE, 0};
    DiskHead(int spacesize):spaceSize(spacesize){LOG_DISK<< "init head";}
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
        LOG_ACTIONSN(id) << "begin action" << headOperator;
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
        LOG_ACTIONSN(id) << "try complete action" << toBeComplete;
        auto action = toBeComplete.action;
        if(toBeComplete.action == NONE){
            return true;
        }
        if(action == READ){
            int Toltimes = toBeComplete.times;
            for(int i=0;i< Toltimes;i++){
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
                    handledOperation->push_back({ READ, i });
                    return false;//无法跨两步读。
                    // //如果没有读开始，但是无法完成
                    // if(i>0){
                    //     handledOperation->push_back({READ, i});//返回已经开始或完成的操作。
                    // }
                    // tokensOffset = readConsume - presentTokens;
                    // presentTokens = 0;
                    // return false;
                }
            }
            handledOperation->push_back({toBeComplete.action, Toltimes});//全部完成
            toBeComplete = {NONE, 0};
            return true;
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
    //把取消的Read都提取出来。其它操作没有必要提取。READ会对ReqUnit造成影响。
    void cancelAction(std::vector<int>* canceledRead) {
        if (this->tokensOffset) {
            this->presentTokens = this->presentTokens - this->tokensOffset;
        }
        int pos = this->headPos;
        if (this->toBeComplete.action = READ) {
            for (int i = 0; i < toBeComplete.times; i++) {
                canceledRead->push_back(this->headPos + i);
            }
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
private:
    //不仅存储对象id(int)，还存储unit值（char*),字节对齐会增加额外空间。
    //也可以用两个space，一个存char，一个存int。
    CircularSpacePiece freeSpace;
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
    void allocate(int objId, int* unitOnDisk, std::vector<int>::iterator it, int start, int len){
        LOG_LINKEDSPACEN(diskId) << "alloc space for obj "<<objId<<
            " on " <<this->diskId<<" (start:"<<start<<" len:"<<len<<")";
        if (objId == 522) {
            int k = 0;
        }
        for(int i=0;i<len;i++){
            int pos = (start+i)%spaceSize;
            diskSpace[pos].objId = objId;
            diskSpace[pos].untId = *it;
            unitOnDisk[*it] = pos;
            LOG_DISKN(diskId)<<"unit "<<*it << " on "<<pos;
            it++;
        }
        if(!freeSpace.testAlloc(start, len)){
            LOG_DISK << "fail: alloc space error";
            throw std::logic_error("test alloc fail!!!");
        }
        LOG_LINKEDSPACEN(diskId) << freeSpace;
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
                LOG_LINKEDSPACEN(diskId) << "free unit on position " << start<<" with len "<<len;
                freeSpace.dealloc(start, len);
                diskSpace[start].objId = 0;//0是被删除的object
                LOG_LINKEDSPACEN(diskId) << "free over";
            }
        }
        LOG_LINKEDSPACE << freeSpace;
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
        this->head.id = id;
        LOG_DISK << "start init space";
        diskSpace.initSpace(spacesize);
        for(int i=0;i<spacesize;i++){
            diskSpace[i].objId = 0;
        }

        LOG_DISK << "malloc space for tagUnitNum";
        this->tagUnitNum = (int*)malloc(sizeof(int)*M);
        for(int i=0;i<M;i++){
            this->tagUnitNum[i] = 0;
        }
        LOG_DISK << "malloc space for tagUnitNum over";
    }
    
    /// @brief 通过存储单元位置返回存储单元信息
    /// @param unitPos 存储单元的位置
    /// @return 返回存储单元的信息，包括存储单元所属的对象Id及其在对象中的位置Id
    DiskUnit getUnitInfo(int unitPos){
        if(unitPos<0||unitPos>=diskSpace.getBufLen()){
            LOG_DISK << "index " << unitPos <<" out of range!";
            throw std::out_of_range("buffer space out of range!");
        }
        return diskSpace[unitPos];
    }
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
        LOG_LINKEDSPACEN(this->diskId) << "start assign space for obj " << obj.objId;
        int objSize = obj.size;
        std::vector<int> units = formUnitsByOrder(objSize, order);////////////////////////
        const SpacePieceNode* node = freeSpace.getStartAfter(head.headPos, true);
        auto tempNode = node;
        if(node->getLen() < objSize){//找到一块合适大小的空间
            tempNode = node->getNext();
            for (int i = 0; i < 20; i++) {//尝试寻找合适大小的空间，尝试最大次数20次
                if (tempNode != node && tempNode->getLen() < objSize) {
                    tempNode = tempNode->getNext();
                }
                else {
                    break;//找不到。
                }
            }
        }

        if(tempNode->getLen() >= objSize){//如果存在能完整放入该对象的空间
            LOG_LINKEDSPACEN(diskId) << "find an whole space, spaceSize " << tempNode->getLen() <<
                    "for space: " << objSize;
            int start = tempNode->getStart();
            if(start==head.headPos){//即如果当前磁头位置处于空白区域的起始处，也即磁头位置没有存储内容。
                int end = (tempNode->getLen()+start)%spaceSize;
                allocate(obj.objId, unitOnDisk, units.begin(), (end-objSize+spaceSize)%spaceSize, objSize);//放在这块空间的尾部
            }else{
                allocate(obj.objId, unitOnDisk, units.begin(), start, objSize);//放在这块空间的首部（与head靠近）
            }
        }else{//找不到一整块足够大小的空间。直接分散存储（效率会较低）。
            //FIXME:应该记录最大空间大小！或者分级存储freeSpace。
            int leftSize = objSize;
            int tempAlloc = 0;
            LOG_LINKEDSPACEN(diskId) << "can't find continuous space. tolSize:" << objSize;
            while(leftSize>0){
                LOG_LINKEDSPACEN(diskId) << "alloc part len:" << node->getLen();
                tempNode = node->getNext();
                tempAlloc = std::min<int>(node->getLen(), leftSize);
                allocate(obj.objId, unitOnDisk, units.begin()+objSize-leftSize, //已经分配的总单元数。
                    node->getStart(), tempAlloc);//在这里，node有可能被删除
                leftSize = leftSize - tempAlloc;
                node = tempNode;
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
    int spaceSize;
    int tokenCost;
    int orgHeadPos;
    int diskId;
    int nextReadConsume;
    std::vector<HeadOperator> actions = {};//对应的行动是什么
    std::vector<int> headPoses = {};//对应的行动会走到哪
    std::vector<int> finishOnTokens = {};//对应的行动在什么时候结束（tokens）
    // HeadActionsInfo(int readConsume, int headpos, int spacesize):
    //     nextReadConsume(readConsume), orgHeadPos(headpos), spaceSize(spacesize){}

    HeadActionsInfo(Disk* disk):
        nextReadConsume(disk->head.readConsume), 
        orgHeadPos(disk->head.headPos), 
        spaceSize(disk->head.spaceSize),
        tokenCost(G - disk->head.presentTokens),//该回合已经花费的tokens
        diskId(disk->diskId)
    {if(disk->head.tokensOffset!=0){
            throw std::logic_error("don't initialize headActionsInfo by head with action incompleted.");
    }}//不要用还未完成行动的head来初始化HeadActionsInfo！
    // 将操作序列附加到现有操作操作序列中，并更新所有操作的累积花费和读操作状态。
    // @param actions 要附加的操作序列。
    int getLastHeadPos(){
        if(headPoses.size() == 0){
            return orgHeadPos;
        }else{
            return headPoses.back();
        }
    }
    int getLastTokenCost() {
        if (finishOnTokens.size()) {
            return finishOnTokens.back();
        }
        else {
            return tokenCost;
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
        if(action == READ){//对于READ操作，一个一个地加。
            auto times = operation.times;
            for(int i=0;i<times;i++){
                freshTokensOnAction(this->nextReadConsume, false);
                freshHeadPoses(1, true);
                this->actions.push_back({READ, 1});

                this->nextReadConsume = std::max<int>(16, std::ceil(0.8 * this->nextReadConsume));
            }
        }else if(action == PASS){
            //PASS不需要细分操作数，因为不需要判断某个单元是否有某个对象在赶往，
            // 只需要知道某个单元什么时候会被读取即可。
            freshTokensOnAction(operation.times, true);
            freshHeadPoses(operation.times, true);
            this->actions.push_back(operation);

            this->nextReadConsume = FIRST_READ_CONSUME;
        }else if(action == JUMP){
            if(operation.jumpTo<0 || operation.jumpTo>=spaceSize){
                throw std::out_of_range("action JUMP jumps out of range");
            }
            freshTokensOnAction(G, false);
            freshHeadPoses(operation.jumpTo, false);
            this->actions.push_back(operation);

            this->nextReadConsume = FIRST_READ_CONSUME;
        }
        else {
            throw std::logic_error("action wrong!");
        }
        
    }
    
    void freshTokensOnAction(int value, bool divisible) {
        if (divisible) {
            this->finishOnTokens.push_back(this->getLastTokenCost() + value);
        }
        else {
            int tolTokens = this->getLastTokenCost();
            if ((tolTokens + value - 1) / G == tolTokens / G + 1) {//如果操作跨回合了。
                tolTokens = (tolTokens / G + 1) * G;//跳到回合开始处。
            }
            tolTokens += value;
            this->finishOnTokens.push_back(tolTokens);
        }
    }
    void freshHeadPoses(int value, bool isOffset){
        if (isOffset) {
            headPoses.push_back((getLastHeadPos() + value) % spaceSize);
        }
        else {
            headPoses.push_back(value);
        }
    }
    // int getTokenCost(){return tokenCost;}
    // int getTimeCost(){return timeCost;}
    // int getNextReadConsume(){return nextReadConsume;}
    // std::vector<std::pair<HeadAction, int>> getActions(){return actions;}
};


LogStream& operator<<(LogStream& s, HeadActionsInfo& headActionsInfo){
    s << "\n";
    s << "diskId:" << headActionsInfo.diskId << " ";
    s << "tokenCost:" << headActionsInfo.tokenCost << " ";
    s << "orgHeadPos:" << headActionsInfo.orgHeadPos << " ";
    s << "nextReadConsume:" << headActionsInfo.nextReadConsume << " ";
    s << "spaceSize:" << headActionsInfo.spaceSize << "\n";
    s << "(actions,topos):\n{";
    for(int i=0;i<headActionsInfo.actions.size();i++){
        s <<"("<< headActionsInfo.actions[i] << headActionsInfo.headPoses[i]<<")";
    }
    s << "}\n";
    return s;
}

class DiskInfo{
    public:
        Disk* disk;
        BplusTree<4> reqSpace;
        std::vector<HeadOperator> handledOperations = {};//作为输出到判题器的参数。
        std::vector<int> completedRead = {};
        std::vector<int> canceledRead = {};
        DiskInfo():disk(nullptr), reqSpace(){
            LOG_DISK << "create diskInfo";
        };
        //行为策略。
        bool toReqUnit(HeadActionsInfo& actionsInfo, int reqUnit){
            LOG_ACTIONSN(actionsInfo.diskId) << "plan to reqUnit:" << reqUnit;
            int distance = disk->getDistance(reqUnit, actionsInfo.getLastHeadPos());//choseRep已经算过了一遍，这里再算一遍？！
            
            if(distance){
                if(distance <= disk->head.presentTokens){
                    //如果可以直接移动到目标位置。
                    actionsInfo.freshActionsInfo(HeadOperator{PASS, distance});
                    return true;//当前时间帧还有可能可以读
                    //也可以试试能不能连读过去（节省时间），这取决于下面有多少单元，能否节省总开销。
                }else if(disk->head.presentTokens == G){
                    //当前还有G个Token，即上一时间步的操作没有延续到当前时间步。可以直接跳过去。
                    actionsInfo.freshActionsInfo(HeadOperator{JUMP, reqUnit});
                    return false;//当前时间帧无法读了。
                }else if(disk->head.presentTokens + G > distance){
                    //如果当前时间步的令牌数不够跳，且这个时间步+下个时间步能够移动到目标位置，
                    //那么先向目标位置移动
                    actionsInfo.freshActionsInfo(HeadOperator{PASS, disk->head.presentTokens});
                    return false;//当前时间帧无法读了。
                }else{
                    //最早也得下两个回合才能到达读位置。
                    //可以视情况做一些有益的策略。（如移动到下回合更可能有请求的位置。
                    //也可以不行动。
                    return false;
                }
            }else{
                return true;//当前就在这个位置。
            }
        }
        //判断是否能够连读。如果能，则连读。
        void multiRead(HeadActionsInfo& actionsInfo){
            int times = 0;
            int reqUnit = actionsInfo.getLastHeadPos();
            LOG_ACTIONSN(actionsInfo.diskId) << "test multiRead from " << reqUnit;
            DiskUnit unitInfo = this->disk->getUnitInfo(reqUnit);
            //如果reqUnit还未被规划，则规划，且查看是否可以连读。
            Object* obj = sObjectsPtr[unitInfo.objId];
            while(obj != &deletedObject &&//该处有对象且未被删除
                        obj->unitReqNum[unitInfo.untId]>0&& //判断是否有请求
                            (!obj->isPlaned(unitInfo.untId))){//判断是否未被规划
                LOG_DISK << "hold read on unit "<< 
                    reqUnit<<" from obj " << unitInfo.objId << " unitOrder " << unitInfo.untId;
                //持续一回合的plan。commitPlan中会有持续多个回合的plan
                obj->plan(unitInfo.untId, this->disk->diskId);
                times ++;
                actionsInfo.freshActionsInfo({ READ, 1 });
                if (actionsInfo.getLastTokenCost() > 2 * G) { break; }//超过两回合就退出。
                reqUnit = (reqUnit+1)%this->disk->spaceSize; //查看下一个位置是否未被规划。
                unitInfo = this->disk->getUnitInfo(reqUnit);
                obj = sObjectsPtr[unitInfo.objId];
            }
            if (times == 0) {
                throw std::logic_error("error! times equals 0. this unit should has not been planed! ");
            }
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
            LOG_DISK << "reqUnit:" << reqUnit;
            auto diskUnit = disk->getUnitInfo(reqUnit);
            while(sObjectsPtr[diskUnit.objId]->isPlaned(diskUnit.untId)){
                reqUnit = this->reqSpace.getNextKeyByAnchor(true);
                if (reqUnit == -1) {
                    break;
                }
                else {
                    diskUnit = disk->getUnitInfo(reqUnit);
                }
            }//直到找到一个还未被规划的unit
            if (!(reqUnit == -1 || (!sObjectsPtr[diskUnit.objId]->isPlaned(diskUnit.untId)))) {
                throw std::logic_error("disk unit should have not been planed");
            }
            return reqUnit;
        }
        bool hasRequestUnit(){
            if(this->reqSpace.root->keyNum>0){
                return true;
            }
            return false;
        }
    };

/// @brief 负责：从请求单元链表中删除已读取的请求单元
///
///请求的信息包括：diskInfo中的reqSpace和actionsPlan、object中的planUnit、
class DiskManager{
public:

    std::vector<DiskInfo*> diskGroup;
    std::vector<int> doneRequestIds;
public:
    //存储一个活动对象的id索引。方便跟进需要查找的单元信息。
    DiskManager(){};
    void addDisk(int spaceSize){
        LOG_DISK << "add disk, space size:" << spaceSize;
        DiskInfo* diskInfo = new DiskInfo();
        LOG_DISK << "create diskInfo over" << spaceSize;
        diskInfo->disk = new Disk(diskGroup.size(), spaceSize);
        diskInfo->reqSpace.id = diskGroup.size();
        LOG_DISK << "create disk over " << spaceSize;
        diskGroup.push_back(diskInfo);
    }
    ~DiskManager(){
        for(int i=0;i<diskGroup.size();i++){
            delete diskGroup[i];
        }
    }
    void freshNewReqUnits(const Object& obj, std::vector<int> unitsOrder){
        bool test = false;
        for(int r=0;r<REP_NUM;r++){//第几个副本
            DiskInfo* disk = this->diskGroup[obj.replica[r]];
            for(int i=0;i<unitsOrder.size();i++){
                int diskId = obj.replica[r];
                int unitOrder = unitsOrder[i];
                diskGroup[diskId]->reqSpace.insert(obj.unitOnDisk[r][unitOrder]);
                test = true;
                LOG_BplusTreeN(diskId)<<" insert unit req"<<obj.unitOnDisk[r][unitOrder];
            }
        }
        if(test){
            LOG_BplusTree << "\n\ninsert unit of obj "<<obj.objId << " to reqSpace";
            for(int i=0;i<REP_NUM;i++){
                LOG_BplusTreeN(obj.replica[i]) << "over insert";
                LOG_BplusTreeN(obj.replica[i]) << *diskGroup[obj.replica[i]]->reqSpace.root;
                LOG_BplusTreeN(obj.replica[i]) << "\n(num:" << diskGroup[obj.replica[i]]->reqSpace.keyNum << ")";

                LOG_BplusTreeN(obj.replica[i]) << "over insert unit of obj " << obj
                     <<" tree:" << diskGroup[obj.replica[i]]->reqSpace;
            }
        }

        
    }
    void freshOvertimeReqUnits(const Object& obj, std::vector<int> unitsOrder){
        for(int r=0;r<REP_NUM;r++){//第几个副本
            DiskInfo* disk = this->diskGroup[obj.replica[r]];
            for(int i=0;i<unitsOrder.size();i++){
                LOG_DISK << "remove overtime req unit "<< obj.unitOnDisk[r][unitsOrder[i]]<<" of disk "<<obj.replica[r];
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
        LOG_DISK << "using strategy max free space size first.";
        int* diskSort = (int*)malloc(sizeof(int)*diskGroup.size());
        for(int i=0;i<diskGroup.size();i++){
            diskSort[i] = i;
        }
        LOG_DISK << "sort disk by free space size";
        std::sort<int*>(diskSort, diskSort+diskGroup.size(), [=](int a, int b){
            return (diskGroup[a]->disk->getFreeSpaceSize() > diskGroup[b]->disk->getFreeSpaceSize());
        });
        LOG_DISK << "assign space to disk:"<<diskSort[0]<<","<<diskSort[1]<<","<<diskSort[2];
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
        LOG_DISK << "timestamp "<< Watch::getTime()<<" plan:";
        std::vector<HeadActionsInfo*> vHeads;
        DiskInfo* diskInfo;
        Disk* disk;
        for(int i=0;i<this->diskGroup.size();i++){
            diskInfo = this->diskGroup[i];
            disk = diskInfo->disk;
            if(disk->head.completeAction(&diskInfo->handledOperations, &diskInfo->completedRead)){
                LOG_DISK << "disk "<< disk->diskId<<" completeAction";
                if(diskInfo->roundReqUnitToPos(disk->head.headPos)){
                    LOG_ACTIONSN(disk->diskId) << "round disk to head pos "<<disk->head.headPos
                            <<", present anchor pos "<<
                        diskInfo->reqSpace.anchor.startKey
                        <<diskInfo->reqSpace.getNextKeyByAnchor(false);
                    vHeads.push_back(new HeadActionsInfo(disk));
                }
                else {
                    LOG_ACTIONSN(i) << "disk "<<i<<" has no reqUnit";
                }
            }
        }
        auto lambdaCompare = [](HeadActionsInfo*& a, HeadActionsInfo*& b){
                return a->getLastTokenCost() < b->getLastTokenCost();//谁当前cost最多就先规划谁。
        };
        std::sort(vHeads.begin(), vHeads.end(), lambdaCompare);
        while(vHeads.size() && vHeads.back()->getLastTokenCost() <G) {//规划一个回合。
            auto vHead = vHeads.back();
            diskInfo = this->diskGroup[vHead->diskId];
            disk = diskInfo->disk;
            //LOG_DISK << "present head:" << *vHead;
            int unitPos = diskInfo->getNextUnplanedReqUnit();
            LOG_DISK << "present head:" << *vHead;
            LOG_DISK << "next unit pos:" << unitPos;
            if(unitPos>=0){
                auto unitInfo = disk->getUnitInfo(unitPos);
                LOG_ACTIONSN(vHead->diskId) << "plan disk to unplaned reqUnit, [pos(" 
                    << unitPos << "):obj(" << unitInfo.objId << "):unt(" << unitInfo.untId << ")";
                LOG_ACTIONSN(vHead->diskId) << "obj info:" << *sObjectsPtr[unitInfo.objId];
                if (diskInfo->toReqUnit(*vHead, unitPos)) {//只有当前时间帧能到达目标位置才能在目标位置开始读。
                    diskInfo->multiRead(*vHead);
                }
                else {//当前时间帧无法到达下一个目标位置。确认当前规划的所有请求。

                }
                LOG_DISK << "head plan:" << *vHead;
                //如果规划完成则将其删除。
                if(vHead->getLastTokenCost() >=G){
                    LOG_ACTIONSN(vHead->diskId) << "commit plan for:" << *vHead;
                    commitPlan(vHead);
                    vHeads.pop_back();//移除最后一个，不会改变顺序
                }
            }else{//如果请求已经转完一圈了就停止。
                LOG_ACTIONSN(vHead->diskId) << "commit plan for:" << *vHead;
                commitPlan(vHead);
                vHeads.pop_back();//不会改变顺序
            }
            std::sort(vHeads.begin(), vHeads.end(), lambdaCompare);
        }
        LOG_DISK << "plan over";
    }

    //在请求单元链表中删除obj的所有第unitOrder个unit的副本。
    void removeObjectReqUnit(const Object& obj, int unitOrder){
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj.replica[i];
            int unitPos = obj.unitOnDisk[i][unitOrder];
            LOG_BplusTreeN(diskId) << "\n\nremove obj " << obj.objId << " done request unit "<<unitPos <<" of disk " <<diskId;
            diskGroup[diskId]->reqSpace.remove(unitPos);
        }
    }
    //执行diskId的已有规划
    //将实际开始执行的行动记录到diskInfo->handledOperations中。
    //将实际执行完成的请求记录到doneRequestIds中。
    void commitPlan(HeadActionsInfo* headActions){//std::vector<HeadOperator> actionsPlan){
        std::vector<HeadOperator>& actionsPlan = headActions->actions;
        int diskId = headActions->diskId;
        if(actionsPlan.size() == 0){
            return;
        }
        if (diskId == -1) {
            throw std::logic_error("a headActions should only be used once");
        }
        DiskInfo* diskInfo = this->diskGroup[diskId];
        Disk* disk = diskInfo->disk;
        //首先completeAction，完成上一次没有完成的持续行动。
        //如果行动在这一回合还是没有完成，那么就接收两个参数。
        //应当在之前先对所有disk试试completeAction。如果返回false，那么此时的presentTokens肯定为0
        LOG_DISK << "test commit plan for disk "<< diskId <<"\nplan:"<<actionsPlan;
        if(disk->head.completeAction(&diskInfo->handledOperations, &diskInfo->completedRead)){
            int planNum = actionsPlan.size();
            for(int i=0;i<planNum;i++){
                LOG_DISK << "loop continue";
                auto action = actionsPlan[i];
                if(!disk->head.beginAction(action)){
                    LOG_DISK << "begin fail, some actions to be completed";
                    break;//要么是之前行动没有做完，要么是输入参数出了问题。
                }else{
                    if(action.action == READ){
                        int actionFinishStep = headActions->finishOnTokens[i]/G;
                        int pos;
                        if (i == 0) {
                            pos = headActions->orgHeadPos;
                        }
                        else {
                            pos = headActions->headPoses[i - 1];
                        }
                        
                        auto unitInfo = disk->getUnitInfo(pos);
                        sObjectsPtr[unitInfo.objId]->plan(unitInfo.untId, diskId, actionFinishStep);
                    }
                    LOG_DISK << "disk "<<disk->diskId<<" begin action:"<<action;
                    if(!disk->head.completeAction(&diskInfo->handledOperations, &diskInfo->completedRead)){
                        break;//剩下的时间片不足以完成这个行动
                    }
                    LOG_DISK << "disk "<<disk->diskId<<" complete action:"<<action;
                    //剩下的时间片完成了这个行动。继续迭代以喂入下一个活动。
                }
            }
        }
        headActions->diskId = -1;
    }
    void freshDoneRequestIds(Disk* disk, std::vector<int> completedRead){
        for(int i=0;i<completedRead.size();i++){
            int readUnit = completedRead[i];
            auto unitInfo = disk->getUnitInfo(readUnit);
            Object* obj = sObjectsPtr[unitInfo.objId];
            //如果这回合刚读完之前的一个行动，但是这回合对应的对象被删除了，那就无效了。
            if(obj != &deletedObject){
                for (int i = 0; i < REP_NUM;i++) {
                    int diskId = obj->replica[i];
                    LOG_BplusTreeN(diskId) << "on read " << readUnit << " remove unit of " << *obj;
                }
                obj->commitUnit(unitInfo.untId, &doneRequestIds);
                this->removeObjectReqUnit(*obj, unitInfo.untId);
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
            LOG_ACTIONSN(i) << "complete read" << diskGroup[i]->completedRead;
            if (diskGroup[i]->completedRead.size()) {
                LOG_BplusTree << "disk " << i << " " << diskGroup[i]->disk->diskId <<
                    " complete read " << diskGroup[i]->completedRead;
                
                freshDoneRequestIds(diskGroup[i]->disk, std::move(diskGroup[i]->completedRead));
            }
        }
        return std::move(doneRequestIds);
    }

    void freeSpace(Object& obj){
        LOG_DISK << "free space of obj "<<obj.objId;
        //如果存在请求则删除该请求。
        for (int j = 0; j < obj.size; j++) {
            if (obj.unitReqNum[j] > 0 && obj.isPlaned(j)) {
                //该单元已被plan，但是将被删除。很可能造成出错，所以如果存在这种情况，应该无视某些读完成的消息？
                // 但是如果后面又有东西在相应规划期内读它，又会造成两个磁头重复读一个磁盘。
                //要不试试取消该磁头的当前读操作？但是又得取消该磁头读取其它obj的操作，而其它obj的某个plan的time可能已经被设置到了某个位置。
                //diskGroup[diskId]->ignoreRead.push_back(obj.unitOnDisk[i][j]);
                
                //取消当前磁头的操作
                int planDiskId = obj.planReqUnit[j];//找到规划该单元的磁头
                LOG_DISKN(planDiskId) << "when delete obj " << obj << " ,obj has been planed";
                Disk* disk = diskGroup[planDiskId]->disk;
                std::vector<int> canceledRead;
                disk->head.cancelAction(&canceledRead);
                LOG_DISKN(planDiskId) << "get canceled read:" << canceledRead;
                for (int c = 0; c < canceledRead.size(); c++) {
                    auto unitInfo = disk->getUnitInfo(canceledRead[c]);
                    auto objPtr = sObjectsPtr[unitInfo.objId];
                    objPtr->clearPlaned(unitInfo.untId);
                    LOG_DISKN(planDiskId) << "fresh obj " << *objPtr << " ,obj has been planed";
                }
            }
            if (obj.unitReqNum[j] > 0) {
                this->removeObjectReqUnit(obj, j);
            }
        }
        for (int j = 0; j < obj.size; j++) {
            if (obj.unitReqNum[j] > 0 && obj.isPlaned(j)) {
                throw std::logic_error("obj has planed unit!!please delete it first!!");
            }
        }
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj.replica[i];
            Disk* disk = diskGroup[diskId]->disk;
            LOG_DISK << "release space in disk "<<diskId;
            LOG_LINKEDSPACEN(diskId) << "free space for obj:" << obj;
            
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