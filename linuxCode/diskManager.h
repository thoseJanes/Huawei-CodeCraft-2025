#ifndef DISKMANAGER_H
#define DISKMANAGER_H
#include "disk.h"
#include "headPlanner.h"

typedef BplusTree<4> BpTree;

class DiskProcessor{
    public:
        Disk* disk;//其中的磁头用于严格执行并返回信息。

        BpTree reqSpace;//请求单元组成的B+树
        HeadPlanner* planner;//优化用磁头规划器，用于临时规划
        std::vector<HeadOperator> handledOperations = {};//已经开始处理的动作，作为输出到判题器的参数。
        std::vector<int> completedRead = {};//被头完成的读请求，作为输出到判题器的参数
        
        DiskProcessor(Disk* diskPtr):disk(diskPtr), reqSpace(){
            planner = new HeadPlanner(diskPtr);
            LOG_DISK << "create diskPcs";
        };
        
        // //行为策略。单步规划。
        // bool toReqUnit(HeadPlanner& headPlanner, int reqUnit){
        //     LOG_ACTIONSN(headPlanner.diskId) << "plan to reqUnit:" << reqUnit;
        //     int distance = disk->getDistance(reqUnit, headPlanner.getLastHeadPos());//choseRep已经算过了一遍，这里再算一遍？！
            
        //     if(distance){
        //         if(distance <= disk->head.presentTokens){
        //             //如果可以直接移动到目标位置。
        //             headPlanner.freshActionsInfo(HeadOperator{PASS, distance});
        //             return true;//当前时间帧还有可能可以读
        //             //也可以试试能不能连读过去（节省时间），这取决于下面有多少单元，能否节省总开销。
        //         }else if(disk->head.presentTokens == G){
        //             //当前还有G个Token，即上一时间步的操作没有延续到当前时间步。可以直接跳过去。
        //             headPlanner.freshActionsInfo(HeadOperator{JUMP, reqUnit});
        //             return false;//当前时间帧无法读了。
        //         }else if(disk->head.presentTokens + G > distance){
        //             //如果当前时间步的令牌数不够跳，且这个时间步+下个时间步能够移动到目标位置，
        //             //那么先向目标位置移动
        //             headPlanner.freshActionsInfo(HeadOperator{PASS, disk->head.presentTokens});
        //             return false;//当前时间帧无法读了。
        //         }else{
        //             //最早也得下两个回合才能到达读位置。
        //             //可以视情况做一些有益的策略。（如移动到下回合更可能有请求的位置。
        //             //也可以不行动。
        //             return false;
        //         }
        //     }else{
        //         return true;//当前就在这个位置。
        //     }
        // }

        
        //判断是否能够连读。如果能，则连读。
        //该函数可以找到不同位置开始的较长的连读段。适合jump时使用。
        bool planMultiRead(HeadPlanner& headPlanner) {//每次选择一个请求单元最多的开始规划？
            int searchNum = 0;
            auto iter = getReqUnitIteratorUnplanedAt(headPlanner.getLastActionNode().endPos);
            if (iter.isEnd()) { return false; }
            std::vector<std::pair<int, int>> readBlocks = {};

            int startReqUnit = iter.getKey();
            if (headPlanner.getDistance(startReqUnit, headPlanner.getLastActionNode().endPos) >= G) {
                int reqUnit = startReqUnit;//第一个请求位置
                double maxProfit = 0;
                double tempProfit = 0;
                while (true) {//对于跳读而言，找到一个最长的读，然后从此处开始判断是否连读。
                    std::vector<std::pair<int, int>> tempReadBlocks = {};
                    auto info = getReadProfitUntilJump(headPlanner, reqUnit, tempReadBlocks);
                    tempProfit = info.first*1.0 / info.second;
                    int nextStart = (tempReadBlocks.back().second + tempReadBlocks.back().first) % this->disk->spaceSize;
                    if(maxProfit < tempProfit){
                        readBlocks = std::move(tempReadBlocks);
                        maxProfit = tempProfit;
                    }
                    if (++searchNum >= MULTIREAD_SEARCH_NUM) {
                        break;
                    }
                    iter = getReqUnitIteratorUnplanedAt(nextStart);
                    if (iter.isEnd()) { throw std::logic_error("既然已经进入了循环，iter就不会到end"); }
                    reqUnit = iter.getKey();//获取下一个未规划的单元。
                    if (startReqUnit == reqUnit) {//如果转了一圈
                        break;
                    }
                }
            }
            else {//判断是继续读还是直接跳。
                int passTokens = headPlanner.getDistance(startReqUnit, headPlanner.getLastActionNode().endPos);
                auto passProfitInfo = getReadProfitUntilJump(headPlanner, startReqUnit, readBlocks);

                int nextReqUnit = readBlocks.back().first + readBlocks.back().second;
                auto iter = getReqUnitIteratorUnplanedAt(nextReqUnit);
                //检查跳是否能取得更高收益。
                int reqUnit = iter.getKey();
                int maxProfit = 0;
                while (true) {//对于跳读而言，找到一个最长的读，然后从此处开始判断是否连读。
                    std::vector<std::pair<int, int>> tempReadBlocks = {};
                    auto info = getReadProfitUntilJump(headPlanner, reqUnit, tempReadBlocks);
                    int passLoss = passProfitInfo.first * passTokens + info.first * (passTokens + passProfitInfo.second + G);
                    int jumpLoss = G * info.first + (2 * G + info.second) * passProfitInfo.first;
                    int nextStart = (tempReadBlocks.back().second + tempReadBlocks.back().first) % this->disk->spaceSize;
                    if (passLoss - jumpLoss > maxProfit) {
                        maxProfit = passLoss - jumpLoss;
                        readBlocks = std::move(tempReadBlocks);
                    }
                    if (++searchNum >= MULTIREAD_SEARCH_NUM) {
                        break;
                    }
                    iter = getReqUnitIteratorUnplanedAt(nextStart);
                    if (iter.isEnd()) { throw std::logic_error("既然已经进入了循环，iter就不会到end"); }
                    reqUnit = iter.getKey();//获取下一个未规划的单元。
                    if (nextReqUnit == reqUnit) {//如果转了一圈
                        break;
                    }
                }
            }
            
            
            headPlanner.appendMoveTo(readBlocks.front().first);
            LOG_PLANNERN(headPlanner.getDiskId()) << "\nbefore add, planner:" << headPlanner;
            LOG_PLANNER << "planner " << this->disk->diskId << " " << headPlanner.getDiskId() <<" planning ";
            for (int k = 0; k < readBlocks.size(); k++) {
                int start = readBlocks[k].first;
                //但是只把当前时间步相关的行动入栈并等待执行。
                if (Watch::toTimeStep(headPlanner.getLastActionNode().endTokens) <= Watch::getTime()) {
                    //以readBlock为单元加入读。
                    for (int j = 0; j < readBlocks[k].second; j++) {
                        headPlanner.appendMoveToAllReadAndPlan((start + j) % disk->spaceSize);
                        LOG_PLANNER  << " plan for unit "
                            << (start + j) % disk->spaceSize;
                        LOG_PLANNERN(headPlanner.getDiskId()) << " plan for unit "
                            << (start + j) % disk->spaceSize;
                    }
                }
                else {
                    for (int j = 0; j < readBlocks[k].second; j++) {
                        auto info = disk->getUnitInfo((start + j) % disk->spaceSize);
                        auto obj = sObjectsPtr[info.objId];
                        obj->plan(info.untId, disk->diskId);//为当前时间步规划。防止其它磁盘也用到该磁盘现在规划的单元。
                        LOG_OBJECT << "obj " << obj->objId << " plan for unit " 
                            << info.untId << " in pos " << (start + j) % disk->spaceSize << " on disk " << this->disk->diskId;
                        LOG_PLANNER << "obj " << obj->objId << " plan for unit "
                            << info.untId << " in pos " << (start + j) % disk->spaceSize << " on disk " << this->disk->diskId;
                    }
                }
            }//也可以直接执行，然后清除未执行完的内容，防止plan的麻烦。
            LOG_PLANNERN(headPlanner.getDiskId()) << "after add, planner:" << headPlanner;
            return true;
        }
        
        //profit用请求数来计算？还是用分数来计算？先用请求数试试。
        //返回{ tolReqNum , tolTokensCost }。计算了移动到第一个起始请求的花费。
        int getMultiReadBlock(int reqUnit, int* getReqNum, int* getTokensCost) {
            int tempPos = reqUnit;

            int readLength = 0;
            int tolMultiReadLen = 0;
            int tolValidReqNum = 0;
            int tolTokensCost = 0;
            int tolScore = 0;
            int maxMultiReadBlockLength = 0;
            int maxMultiReadValidLength = 0;
            int maxMultiReadValidReqNum = 0;
            int maxMultiReadTokensCost = 0;
            DiskUnit unitInfo = this->disk->getUnitInfo(tempPos);
            Object* obj = sObjectsPtr[unitInfo.objId];
            int multiReadTokensProfit = 0;
            int multiReadLen = 0;
            
            int invalidReadLen = 0;
            bool lastJudgeRead = false;
            
            while (invalidReadLen < 8) {//从tempPos开始，是否可以读原本不需要读的块来获取收益。
                readLength++;
                if (obj != &deletedObject && obj != nullptr &&//该处有对象且未被删除
                    obj->unitReqNum[unitInfo.untId] > 0 && //判断是否有请求
                    (!obj->isPlaned(unitInfo.untId))) {
                    lastJudgeRead = true;
                    tolTokensCost += getReadConsumeAfterN(readLength-1);
                    tolValidReqNum += obj->unitReqNum[unitInfo.untId];
                    tolMultiReadLen++;
                    multiReadLen++;
                    invalidReadLen = 0;

                    multiReadTokensProfit = std::min(0, 
                        multiReadTokensProfit
                        + getReadConsumeAfterN(multiReadLen)
                        - getReadConsumeAfterN(readLength));
                }
                else {
                    lastJudgeRead = false;
                    tolTokensCost += getReadConsumeAfterN(multiReadLen-1);
                    invalidReadLen++;
                    multiReadLen = 0;
                    
                    multiReadTokensProfit =
                        multiReadTokensProfit
                        - getReadConsumeAfterN(readLength) + 1;
                }
                if (multiReadTokensProfit >= 0) {//没有连读损耗
                    
                    maxMultiReadBlockLength = readLength;//那么就连读到该位置。
                    maxMultiReadValidLength = tolMultiReadLen;
                    maxMultiReadValidReqNum = tolValidReqNum;
                    maxMultiReadTokensCost = tolTokensCost;
                }
                tempPos = (tempPos + 1) % this->disk->spaceSize; //查看下一个位置是否未被规划。
                unitInfo = this->disk->getUnitInfo(tempPos);
                obj = sObjectsPtr[unitInfo.objId];
            }
            
            *getReqNum = maxMultiReadValidReqNum;
            *getTokensCost = maxMultiReadTokensCost;
            return maxMultiReadBlockLength;
        }
        std::pair<int,int> getReadProfitUntilJump(HeadPlanner& headPlanner, int start,
                std::vector<std::pair<int,int>>& readBlocks) {
            int tolReqNum = 0; int tolTokensCost = 0;
            auto iter = getReqUnitIteratorUnplanedAt(start);
            if (iter.isEnd()) { return {0, 1}; }
            int cost = headPlanner.getDistance(iter.getKey(), start); assert(cost == 0);
            while (!iter.isEnd() && 
                    headPlanner.getDistance(iter.getKey(), start) < G &&
                    readBlocks.size() <= 20) {
                tolTokensCost += headPlanner.getDistance(iter.getKey(), start);//加上pass的距离。
                int reqNum = 0; int tokensCost = 0;
                int length = getMultiReadBlock(iter.getKey(), &reqNum, &tokensCost);
                tolReqNum += reqNum; tolTokensCost += tokensCost;
                readBlocks.push_back({ start, length });

                start = start + length;
                auto iter = getReqUnitIteratorUnplanedAt(start);//会找到下一个或者和start相等的。
            }
            LOG_PLANNER << "read blocks size:" << readBlocks.size();
            return { tolReqNum , tolTokensCost };
        }


        bool simpleMultiRead(HeadPlanner& headPlanner) {
            assert(headPlanner.getDiskId() == this->disk->diskId);
            int readLength = 0;
            auto iter = getReqUnitIteratorUnplanedAt(headPlanner.getLastActionNode().endPos);
            if (iter.isEnd()) { return false; };

            int startReadPos = iter.getKey(); int reqUnit = startReadPos;
            DiskUnit unitInfo = headPlanner.getDisk()->getUnitInfo(reqUnit);
            //如果reqUnit还未被规划，则规划，且查看是否可以连读。
            Object* obj = sObjectsPtr[unitInfo.objId];
            while (obj != &deletedObject &&//该处有对象且未被删除
                obj->unitReqNum[unitInfo.untId] > 0 && //判断是否有请求
                (!obj->isPlaned(unitInfo.untId))) {//判断是否未被规划
                readLength++;
                reqUnit = (reqUnit + 1) % headPlanner.getDisk()->spaceSize; //查看下一个位置是否未被规划。
                unitInfo = headPlanner.getDisk()->getUnitInfo(reqUnit);
                obj = sObjectsPtr[unitInfo.objId];
            }
            for (int i = 0; i < readLength; i++) {
                headPlanner.appendMoveToUnplannedReadAndPlan((startReadPos + i) % headPlanner.getDisk()->spaceSize);
            }
            return true;
        }
        //reqUnit相关
        BpTree::Iterator getIteratorAt(int pos) {
            if (this->reqSpace.getKeyNum() > 0) {
                return this->reqSpace.iteratorAt(pos);
            }
            else {
                return BpTree::Iterator();
            }
        }
        BpTree::Iterator getReqUnitIteratorUnplanedAt(int pos) {
            if (this->reqSpace.getKeyNum() > 0) {
                auto it = this->reqSpace.iteratorAt(pos);
                int reqUnit = it.getKey();
                auto diskUnit = disk->getUnitInfo(reqUnit);
                while (sObjectsPtr[diskUnit.objId]->isPlaned(diskUnit.untId)) {
                    it.toNext();
                    if (it.isEnd()) {
                        return BpTree::Iterator();
                    }
                    reqUnit = it.getKey();
                    diskUnit = disk->getUnitInfo(reqUnit);
                }
                return it;
            }
            else {
                return BpTree::Iterator();
            }
        }
        /// @brief 获取下一个key
        /// @param toNext 是否把锚点移动到下一个key处。
        /// @return 如果已经没有下一个请求单元，则返回-1，否则返回请求单元的位置
        
        int getNextRequestUnit(BpTree::Iterator& iterator, bool toNext = true){
            if (not toNext) {
                auto it = iterator.getNext();
                if (!it.isEnd()) {
                    return it.getKey();
                }
                return -1;
            }
            iterator.toNext();
            if (!iterator.isEnd()) {
                return iterator.getKey();
            }
            return -1;
            // }catch(const std::logic_error& e){
            //     throw;
            // }
        }
        /// @brief 获取下一个未被规划的请求单元，并且把锚点移动到该请求单元处。
        /// @return 如果已经没有下一个未被规划的请求单元，则返回-1，否则返回请求单元的位置
        int toNextUnplanedReqUnit(BpTree::Iterator& iterator){
            iterator.toNext();
            if (iterator.isEnd()) {
                return -1;
            }
            int reqUnit = iterator.getKey();
            LOG_DISK << "reqUnit:" << reqUnit;
            auto diskUnit = disk->getUnitInfo(reqUnit);
            while(sObjectsPtr[diskUnit.objId]->isPlaned(diskUnit.untId)){
                iterator.toNext();
                if (iterator.isEnd()) {
                    return -1;
                }
                reqUnit = iterator.getKey();
                diskUnit = disk->getUnitInfo(reqUnit);
            }//直到找到一个还未被规划的unit
            return reqUnit;
        }
        bool hasRequestUnit(){
            if(this->reqSpace.getRoot()->keyNum>0) {
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
    std::vector<DiskProcessor*> diskGroup;
    std::vector<int> doneRequestIds;
public:
    //存储一个活动对象的id索引。方便跟进需要查找的单元信息。
    DiskManager(){};
    void addDisk(int spaceSize){
        LOG_DISK << "add disk, space size:" << spaceSize;
        LOG_DISK << "create diskPcs over" << spaceSize;
        DiskProcessor* diskPcs = new DiskProcessor(new Disk(diskGroup.size(), spaceSize));
        LOG_DISK << "create disk over " << spaceSize;
        diskGroup.push_back(diskPcs);

        diskPcs->reqSpace.id = diskGroup.size();//用于log
    }
    ~DiskManager(){
        for(int i=0;i<diskGroup.size();i++){
            delete diskGroup[i]->disk;
            delete diskGroup[i];
        }
    }
    
    void freshNewReqUnits(const Object& obj, std::vector<int> unitIds){
        bool test = false;
        for(int r=0;r<REP_NUM;r++){//第几个副本
            DiskProcessor* disk = this->diskGroup[obj.replica[r]];
            for(int i=0;i<unitIds.size();i++){
                int diskId = obj.replica[r];
                int unitId = unitIds[i];
                diskGroup[diskId]->reqSpace.insert(obj.unitOnDisk[r][unitId]);
                test = true;
                LOG_BplusTreeN(diskId)<<" insert unit req"<<obj.unitOnDisk[r][unitId];
            }
        }
        if(test){
            LOG_BplusTree << "\n\ninsert unit of obj "<<obj.objId << " to reqSpace";
            for(int i=0;i<REP_NUM;i++){
                LOG_BplusTreeN(obj.replica[i]) << "over insert";
                LOG_BplusTreeN(obj.replica[i]) << *diskGroup[obj.replica[i]]->reqSpace.getRoot();
                LOG_BplusTreeN(obj.replica[i]) << "\n(num:" << diskGroup[obj.replica[i]]->reqSpace.getKeyNum() << ")";

                // LOG_BplusTreeN(obj.replica[i]) << "over insert unit of obj " << obj
                //      <<" tree:" << diskGroup[obj.replica[i]]->reqSpace;
            }
        }

        
    }
    void freshOvertimeReqUnits(const Object& obj, std::vector<int> unitIds){
        for(int r=0;r<REP_NUM;r++){//第几个副本
            DiskProcessor* diskPcs = this->diskGroup[obj.replica[r]];
            for(int i=0;i<unitIds.size();i++){
                LOG_DISK << "remove overtime req unit "<< obj.unitOnDisk[r][unitIds[i]]<<" of disk "<<obj.replica[r];
                diskPcs->reqSpace.remove(obj.unitOnDisk[r][unitIds[i]]);
            }
        }
    }
    //在请求单元链表中删除obj的所有第unitOrder个unit的副本。
    void removeObjectReqUnit(const Object& obj, int unitId){
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj.replica[i];
            int unitPos = obj.unitOnDisk[i][unitId];
            LOG_BplusTreeN(diskId) << "\n\nremove obj " << obj.objId << " done request unit "<<unitPos <<" of disk " <<diskId;
            diskGroup[diskId]->reqSpace.remove(unitPos);
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
    int choseRep(Object*& obj){
        //计算磁盘离对象有效被请求块的最大距离。//在每个jump命令前尝试规划。
        int dists[REP_NUM];
        for(int i=0;i<REP_NUM;i++){
            dists[i] = 0;
            int diskId = obj->replica[i];
            Disk* disk = diskGroup[diskId]->disk;
            for(int j=0;j<obj->size;j++){
                if(obj->unitReqNum[j]==0){
                    continue;
                }
                dists[i] = std::max<int>(dists[i], 
                    disk->getDistance(obj->unitOnDisk[i][j], disk->head.headPos));
                    //应该用head的当前pos吗？还是用headPlanner来规划?
            }
        }
        auto minPos = static_cast<int>(std::min_element(dists, dists+REP_NUM) - dists);
        return obj->replica[minPos];
    }
    float objectScore(Object*& a){
        return a->score*(1.0/a->getReqUnitSize()+1.0);
    }
    
    //如果能连读就连读，如果无法连读才进行读节点插入。
    void multiReadStrategy() {
        //计算每个单元在对应磁盘上所需的时间。并且判断单元是否和之前的单元在同一磁盘上，
        LOG_DISK << "timestamp " << Watch::getTime() << " plan disk:";
        std::vector<HeadPlanner*> vHeads;
        
        DiskProcessor* diskPcs; Disk* disk;
        //完成上一步行动，并且把请求指针转到当前head位置。
        for (int i = 0; i < this->diskGroup.size(); i++) {
            diskPcs = this->diskGroup[i];
            disk = diskPcs->disk;
            if (disk->head.completeAction(&diskPcs->handledOperations, &diskPcs->completedRead)) {
                LOG_DISK << "disk " << disk->diskId << " completeAction"; 
                if (Watch::toTimeStep(diskPcs->planner->getLastActionNode().endTokens) <= Watch::getTime() + PLAN_STEP - 1
                    && diskPcs->reqSpace.getKeyNum()>0) {
                    vHeads.push_back(diskPcs->planner);
                }
                else {
                    LOG_ACTIONSN(i) << "disk " << i << " has no reqUnit. try to execute";
                }
            }
        }

        LOG_OBJECT << "\nIn planing: present requested objects———— ";
        for (auto it = requestedObjects.begin(); it != requestedObjects.end(); it++) {
            LOG_OBJECT << *(*it);
        }
        LOG_OBJECT << "\n";
        //先规划请求单元多的磁盘。
        auto lambdaCompareHeadPlanner = [this](HeadPlanner*& a, HeadPlanner*& b) {
            return this->diskGroup[a->getDiskId()]->reqSpace.getKeyNum()
                > this->diskGroup[b->getDiskId()]->reqSpace.getKeyNum();//谁当前cost最多就先规划谁。
            };
        while (vHeads.size()) {//loop
            //找到所有磁盘的（相对于磁头）第一个未规划plan,以及对应的object
            std::sort(vHeads.begin(), vHeads.end(), lambdaCompareHeadPlanner);
            for (int i = 0; i < vHeads.size(); i++) {
                auto planner = vHeads[i];
                if (Watch::toTimeStep(planner->getLastActionNode().endTokens) > Watch::getTime() + PLAN_STEP - 1) {
                    vHeads.erase(vHeads.begin() + i);
                    i--;
                    continue;
                }
                int diskId = planner->getDiskId();
                if (!diskGroup[diskId]->planMultiRead(*planner)) {
                    vHeads.erase(vHeads.begin() + i);
                    i--;
                    continue;
                }
            }
        }
        LOG_DISK << "plan over";
        for (int i = 0; i < diskGroup.size(); i++) {
            diskPcs = this->diskGroup[i];
            diskPcs->planner->excutePresentTimeStep(&diskPcs->handledOperations, &diskPcs->completedRead);
        }
        LOG_DISK << "execute over";
    }

    //如果要跳，那么就选择最大价值的object跳。需要跳的规划最后安排。
    void objectBasedReadStrategy(){
        //计算每个单元在对应磁盘上所需的时间。并且判断单元是否和之前的单元在同一磁盘上，
        LOG_DISK << "timestamp "<< Watch::getTime()<<" plan disk:";
        std::vector<std::pair<HeadPlanner*, BpTree::Iterator>> vHeads;
        DiskProcessor* diskPcs; Disk* disk;
        //完成上一步行动，并且把请求指针转到当前head位置。
        for(int i=0;i<this->diskGroup.size();i++){
            diskPcs = this->diskGroup[i];
            disk = diskPcs->disk;
            if(disk->head.completeAction(&diskPcs->handledOperations, &diskPcs->completedRead)){
                LOG_DISK << "disk "<< disk->diskId<<" completeAction";
                auto iter = diskPcs->getReqUnitIteratorUnplanedAt(disk->head.headPos);
                if(Watch::toTimeStep(diskPcs->planner->getLastActionNode().endTokens) <= Watch::getTime() + PLAN_STEP - 1
                    && (!iter.isEnd())){
                    LOG_ACTIONSN(disk->diskId) << "round disk to head pos " << disk->head.headPos
                        << ", present anchor pos ";
                    vHeads.push_back({ diskPcs->planner, iter });
                }
                else {
                    LOG_ACTIONSN(i) << "disk "<<i<<" has no reqUnit. try to execute";
                }
            }
        }
        

        auto lambdaCompare = [this](Object*& a, Object*& b){
            return objectScore(a) > objectScore(b);
            //sort会把true的放前面。也就是把分数大的放在前面。
        };
        std::sort(requestedObjects.begin(), requestedObjects.end(), lambdaCompare);
        LOG_OBJECT << "\nIn planing: present requested objects———— ";
        for (auto it = requestedObjects.begin(); it != requestedObjects.end(); it++) {
            LOG_OBJECT << *(*it);
        }
        LOG_OBJECT << "\n";
        std::vector<int> unitPoses;
        std::vector<int> dists;
        std::vector<Object*> accessibleObjects;
        //先规划工期短的磁盘。如果工期都超过当前3个时间步，则先规划请求数少的磁盘。(或者先规划空行动最多的磁盘)
        auto lambdaCompareHeadPlanner = [this](std::pair<HeadPlanner*, BpTree::Iterator>& a, std::pair<HeadPlanner*, BpTree::Iterator>& b){
            //if(Watch::toTimeStep(a->getLastActionNode().endTokens) > Watch::getTime() + 2){
            //    if(Watch::toTimeStep(b->getLastActionNode().endTokens) > Watch::getTime() + 2){
            //        return diskGroup[a->getDiskId()]->reqSpace.getKeyNum() > 
            //                diskGroup[b->getDiskId()]->reqSpace.getKeyNum();//谁keyNum最少就先规划谁。
            //    }else{
            //        return true;//把b放后面
            //    }
            //}
            return a.first->getLastActionNode().endTokens < b.first->getLastActionNode().endTokens;//谁当前cost最多就先规划谁。
        };
        while(vHeads.size()){//loop
            unitPoses = {};
            dists = {};
            //找到所有磁盘的（相对于磁头）第一个未规划plan,以及对应的object
            for(int i=0;i<vHeads.size();i++){
                auto planner = vHeads[i].first;
                if(Watch::toTimeStep(planner->getLastActionNode().endTokens) > Watch::getTime() + PLAN_STEP - 1){
                    auto diskPcs = diskGroup[planner->getDiskId()];
                    vHeads.erase(vHeads.begin()+i);
                    i--;
                    continue;
                }
                
                if(vHeads[i].second.isEnd()){//找不到下一个位置。
                    auto diskPcs = diskGroup[planner->getDiskId()];
                    //执行时间不应该放在这里！可能在别人的规划里获得行动！！
                    vHeads.erase(vHeads.begin()+i);
                    i--;
                    continue;
                }else{
                    int unitPos = vHeads[i].second.getKey();
                    int dist;
                    planner->getShortestDistance(unitPos, &dist);
                    dists.push_back(dist);
                    unitPoses.push_back(unitPos);
                }

                int diskId = planner->getDiskId();
                diskGroup[diskId]->toNextUnplanedReqUnit(vHeads[i].second);
            }
            std::sort(vHeads.begin(), vHeads.end(), lambdaCompareHeadPlanner);
            //找到可达plan对应的对象。
            accessibleObjects = {};
            for(int i=0;i<vHeads.size();i++){
                auto planner = vHeads[i].first;
                if(dists[i] < G){
                    int objId = planner->getDisk()->getUnitInfo(unitPoses[i]).objId;
                    accessibleObjects.push_back(sObjectsPtr[objId]);//按 价值/距离 规划？
                }else{
                    //不可达。
                    accessibleObjects.push_back(nullptr);
                }
            }
            //按原排序顺序规划。
            for(int i=0;i<vHeads.size();i++){
                auto planner = vHeads[i].first;
                if(accessibleObjects[i] == nullptr){
                    planObjectsRead(findFirstUnplannedObject(requestedObjects, planner->getDiskId()));
                }else{
                    planObjectsRead(accessibleObjects[i]);
                }
            }
            //删除时间片足够的对象。在while开始时会删除。
        }
        LOG_DISK << "plan over";
        for (int i = 0; i < diskGroup.size(); i++) {
            diskPcs = this->diskGroup[i];
            diskPcs->planner->excutePresentTimeStep(&diskPcs->handledOperations, &diskPcs->completedRead);
        }
        LOG_DISK << "execute over";
    }

    Object* findFirstUnplannedObject(std::vector<Object*>& objVec, int diskId){
        for(int i=0;i<objVec.size();i++){
            //优化方向：只判断未被规划部分的分数价值
            if(!objVec[i]->allPlaned()){
                if(std::find(objVec[i]->replica, objVec[i]->replica+REP_NUM, diskId) 
                    != objVec[i]->replica+REP_NUM);
                return objVec[i];
            }
        }
        return nullptr;
    }

    void planObjectsRead(Object* obj){
        if(obj == nullptr){
            return;
        }
        if (obj->allPlaned()) {
            return;
        }
        LOG_PLANNER << "\nplaning object:" << *obj;
        //规划大分对象的磁盘，考虑因素：能否最快地完成这个对象。
        int earliest = Watch::getTime();//这个earliest表示的是前面单元最快完成时间步。
        for(int i=0;i<obj->size;i++){//以请求单元上累积的请求数从大到小的顺序规划.小者会被大者阻塞。
            int unitId = obj->unitReqNumOrder[i];
            if (unitId == 3&&obj->objId == 3245&&Watch::getTime()== 8855) {
                int test = 0;
            }
            if(!obj->isPlaned(unitId) && obj->unitReqNum[unitId]>0) {//或者if(!obj->isPlanned())，来选择未规划的单元。
                planUnitsRead(obj, obj->unitReqNumOrder, i, &earliest);
            }
            else {
                earliest = std::max<int>(earliest, obj->planReqTime[unitId]);
            }
        }
        for (int i = 0; i < REP_NUM; i++) {
            LOG_PLANNER << "disk" << obj->replica[i] <<" plan:\n" << diskGroup[obj->replica[i]]->planner->getActionNodes();
        }
        LOG_PLANNER << "planing object over, object:" << *obj;
    }
    
    /// @brief 规划对象的单元
    /// @param obj 存储对象
    /// @param unitReqNumOrder 存储对象中单元的请求数量排序，存储单元的id，以单元上的请求数从大到小排列
    /// @param place 当前规划的单元在unitReqNumOrder中的位置，也即规划第几大的单元。
    /// @param aheadEarliest 排在前面的单元的最晚完成时间步。也就是当前单元的最早完成时间步。
    void planUnitsRead(Object* obj, int* unitReqNumOrder, int place, int* aheadEarliest){
        //在place前面的请求都制约着阻塞在place处请求的完成时间。
        //可以尝试在这里改进前面的请求单元，如果不在同一个请求单元上，也可以尝试多次循环规划来改进。
        //在这里判断得分需要用到。第一个得分会成为基准得分？否，最快得分作为基准得分。
        int scoreLoss;//一般是一个正数
        int scoreGain;//一般是一个负数（和最大得分的距离）
        int tolScore[REP_NUM];
        int readOverTokens[REP_NUM];
        int tolTokens[REP_NUM];
        int maxTolScore;
        int diskSelected;
        int unitId = unitReqNumOrder[place];
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj->replica[i];
            auto actionPlanner = diskGroup[diskId]->planner;
            scoreLoss = 0; scoreGain = 0;
            actionPlanner->insertUnplannedReadAsBranch(obj->unitOnDisk[i][unitId], readOverTokens+i, &scoreLoss);
            tolTokens[i] = actionPlanner->getLastActionNode().endTokens;
            //place的plan不会影响place之前的，但是会影响它之后的，并且如果它之后有已经规划的单元，那么影响取决于短板效应。
            int tempEarliest = *aheadEarliest;
            for (int j = place; j < obj->size; j++) {
                if (obj->isPlaned(unitReqNumOrder[j])) {
                    tempEarliest = std::max<int>(tempEarliest, obj->planReqTime[unitReqNumOrder[j]]);
                }
                scoreGain += std::min<int>(tempEarliest - Watch::toTimeStep(readOverTokens[i]), 0) * obj->coEdgeValue[j];
            }

            assert(scoreGain <= 0);
            tolScore[i] = scoreGain - scoreLoss;//为负数。
            assert(tolScore[i] <= 0);
            LOG_PLANNERN(diskId) << "planing unit " << unitId << " of obj " << obj->objId
                << ",scoreLoss:" << scoreLoss << ", scoreGain:" 
                << scoreGain <<",tolScore:"<< tolScore[i] << ", coValue:" << place + 1;
            LOG_PLANNER << "planing unit " << unitId << " of obj " << obj->objId
                << " on disk " << diskId << ",scoreLoss:" << scoreLoss << ", scoreGain:" 
                << scoreGain << ",tolScore:" << tolScore[i] <<", coValue:"<<place+1;
            //记录分数最大的副本（优化方向：可以改为规划最长长度(有上限)和分数加权为总分数）
            if(i==0){
                diskSelected = diskId;
                maxTolScore = tolScore[0];
            }else if(tolScore[i]>maxTolScore){
                maxTolScore = tolScore[i];
                diskSelected = diskId;
            }else if(tolScore[i] == maxTolScore && tolTokens[i] < tolTokens[diskSelected]){
                //如果分数相等，则选择目前规划长度更小的那个规划。
                maxTolScore = tolScore[i];
                diskSelected = diskId;
            }
        }

        //简单地选择这一单元规划后分数最大的。
        *aheadEarliest = std::max<int>(*aheadEarliest, Watch::toTimeStep(readOverTokens[diskSelected]));
        LOG_PLANNERN(diskSelected) << "get the unit of obj " << obj->objId;
        LOG_PLANNER << "disk" << diskSelected << " get the unit of obj " << obj->objId;
        for(int i=0;i<REP_NUM;i++){
            int unitPos = obj->unitOnDisk[i][unitId];
            int diskId = obj->replica[i];
            auto actionPlanner = diskGroup[diskId]->planner;
            if(diskId == diskSelected){
                actionPlanner->mergeUnplannedReadBranch(unitPos);//merge时会改变plan值。
            }else{
                actionPlanner->dropReadBranch(unitPos);
            }
        }
    }

    //执行diskId的已有规划
    //将实际开始执行的行动记录到diskInfo->handledOperations中。
    //将实际执行完成的请求记录到doneRequestIds中。
    void commitPlan(HeadPlanner* headPlanner){//std::vector<HeadOperator> actionsPlan){
        const std::list<ActionNode> actionsPlan = headPlanner->getActionNodes();
        int diskId = headPlanner->getDiskId();
        if(actionsPlan.size() == 0){
            return;
        }
        if (diskId == -1) {
            throw std::logic_error("a headPlanner should only be used once");
        }
        DiskProcessor* diskPcs = this->diskGroup[diskId];
        Disk* disk = diskPcs->disk;
        //首先completeAction，完成上一次没有完成的持续行动。
        //如果行动在这一回合还是没有完成，那么就接收两个参数。
        //应当在之前先对所有disk试试completeAction。如果返回false，那么此时的presentTokens肯定为0
        //LOG_DISK << "test commit plan for disk "<< diskId <<"\nplan:"<<actionsPlan;
        if(disk->head.completeAction(&diskPcs->handledOperations, &diskPcs->completedRead)){
            int planNum = actionsPlan.size();
            for(auto it=std::next(actionsPlan.begin(),1);it!=actionsPlan.end();it++){
                LOG_DISK << "loop continue";
                auto actionNode = *it;
                if(!disk->head.beginAction(actionNode.action)){
                    LOG_DISK << "begin fail, some actions to be completed";
                    break;//要么是之前行动没有做完，要么是输入参数出了问题。也可能tokens刚好为0
                }else{
                    if(actionNode.action.action == READ || actionNode.action.action == VREAD){
                        int actionFinishStep = Watch::toTimeStep(actionNode.endTokens);
                        int pos = actionNode.endPos - 1;//因为读操作现在只可能是单步的。
                        
                        auto unitInfo = disk->getUnitInfo(pos);
                        sObjectsPtr[unitInfo.objId]->test_plan(unitInfo.untId, diskId, actionFinishStep, false);
                    }
                    LOG_DISK << "disk "<<disk->diskId<<" begin action:"<<actionNode;

                    if(!disk->head.completeAction(&diskPcs->handledOperations, &diskPcs->completedRead)){
                        break;//剩下的时间片不足以完成这个行动
                    }
                    assert(Watch::toTimeStep(actionNode.endTokens) == Watch::getTime());//时间对齐。
                    LOG_DISK << "disk "<<disk->diskId<<" complete action:"<<actionNode;
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
                int* repId = std::find(obj.replica, obj.replica + REP_NUM, planDiskId);
                assert(static_cast<int>(repId - obj.replica) < REP_NUM);
                int readPos = obj.unitOnDisk[static_cast<int>(repId - obj.replica)][j];

                diskGroup[planDiskId]->planner->cancelRead(readPos);
                LOG_DISKN(planDiskId) << "get canceled read:" << readPos;
                auto unitInfo = disk->getUnitInfo(readPos);
                auto objPtr = sObjectsPtr[unitInfo.objId];
                objPtr->clearPlaned(unitInfo.untId);
                LOG_DISKN(planDiskId) << "fresh obj " << *objPtr << " ,obj plan has been cleared";
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
    const HeadPlanner* getPlanner(int i) const {
        return diskGroup[i]->planner;
    }
};

#endif
