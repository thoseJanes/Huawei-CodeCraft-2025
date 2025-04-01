#ifndef DISKMANAGER_H
#define DISKMANAGER_H
#include <set>
#include "disk.h"
#include "headPlanner.h"

typedef BplusTree<4> BpTree;
struct ReadProfitInfo {
    int score = 0;
    int edge = 0;
    int reqNum = 0;
    int tokensCost = 0;
};

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
        
        
        class ReqIterator {
            BpTree::Anchor iterator;
            Disk* disk;
        public:
            ReqIterator(BpTree::Anchor iterator, Disk* disk) :disk(disk), iterator(iterator) {}
            int getNextRequestUnit(bool toNext = true) {
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
            }
            void toNextUnplanedReqUnit() {
                iterator.toNext();
                if (iterator.isEnd()) {
                    return;
                }
                int reqUnit = iterator.getKey();
                LOG_DISK << "reqUnit:" << reqUnit;
                auto diskUnit = disk->getUnitInfo(reqUnit);
                while (sObjectsPtr[diskUnit.objId]->isPlaned(diskUnit.untId)) {
                    iterator.toNext();
                    if (iterator.isEnd()) {
                        return;
                    }
                    reqUnit = iterator.getKey();
                    diskUnit = disk->getUnitInfo(reqUnit);
                }//直到找到一个还未被规划的unit
            }
            int getKey() {
                return iterator.getKey();
            }
            ReqIterator& toNext() {
                iterator.toNext();
                return *this;
            }
            ReqIterator& toUnplanedNoLessThan(int key) {
                iterator.toNoLessThan(key);
                if (iterator.isEnd()) {
                    return *this;
                }
                auto info = this->disk->getUnitInfo(iterator.getKey());
                auto obj = sObjectsPtr[info.objId];
                if (obj->isPlaned(info.untId)) {
                    toNextUnplanedReqUnit();
                }
                return *this;
            }
            int isEnd() {
                return iterator.isEnd();
            }
            ~ReqIterator() {}
        };
        //reqUnit相关
        ReqIterator getIteratorAt(int pos) {
            return ReqIterator(this->reqSpace.anchorAt(pos), disk);
        }
        ReqIterator getReqUnitIteratorUnplanedAt(int pos) {
            auto it = this->reqSpace.anchorAt(pos);
            if (it.isEnd()) {
                return ReqIterator(it, disk);
            }

            int reqUnit = it.getKey();
            auto diskUnit = disk->getUnitInfo(reqUnit);
            while (sObjectsPtr[diskUnit.objId]->isPlaned(diskUnit.untId)) {
                it.toNext();
                if (it.isEnd()) {
                    return ReqIterator(it, disk);
                }
                reqUnit = it.getKey();
                diskUnit = disk->getUnitInfo(reqUnit);
            }
            return ReqIterator(it, disk);
        }

        int calNextStart(std::vector<std::pair<int, int>>& readBlocks);

        bool planMultiReadByReqNum();
        ReadProfitInfo getReqProfitUntilJump(int start, std::vector<std::pair<int, int>>& readBlocks);
        int getMultiReadBlockAndReqNum(int reqUnit, int* getReqNum, int* getTokensCost);


        bool planMultiReadWithExactInfo();
        ReadProfitInfo getExactReadProfitUntilJump(int start, int startTokens, std::vector<std::pair<int, int>>& readBlocks);
        int getMultiReadBlockAndRelatedObjects(int reqUnit, int startTokens, int* getTokensCost, std::set<Object*>* relatedObjects, int* getReqNum);



        bool simpleMultiRead();
        /// @brief 获取下一个key
        /// @param toNext 是否把锚点移动到下一个key处。
        /// @return 如果已经没有下一个请求单元，则返回-1，否则返回请求单元的位置
        
        
        /// @brief 获取下一个未被规划的请求单元，并且把锚点移动到该请求单元处。
        /// @return 如果已经没有下一个未被规划的请求单元，则返回-1，否则返回请求单元的位置
        
        bool hasRequestUnit(){
            if(this->reqSpace.getRoot()->keyNum>0) {
                return true;
            }
            return false;
        }
    };

/// @brief 负责：从请求单元链表中删除已读取的请求单元
///请求的信息包括：diskInfo中的reqSpace和actionsPlan、object中的planUnit、
class DiskManager{
public:
    std::vector<DiskProcessor*> diskGroup;
    std::vector<int> doneRequestIds;
    std::map<int, std::vector<Disk*>> tagToDisks;
public:
    //存储一个活动对象的id索引。方便跟进需要查找的单元信息。
    DiskManager(int tagNum){
        for (int i = 0; i < tagNum; i++) {
            tagToDisks[i] = {};
        }
    };
    void addDisk(int spaceSize, std::vector<std::pair<int,int>>& tagToSpaceSize){
        LOG_DISK << "add disk, space size:" << spaceSize;
        LOG_DISK << "create diskPcs over" << spaceSize;
        auto disk = new Disk(diskGroup.size(), spaceSize, tagToSpaceSize);
        for (int i = 0; i < tagToSpaceSize.size(); i++) {
            tagToDisks.at(tagToSpaceSize[i].first).push_back(disk);
        }
        DiskProcessor* diskPcs = new DiskProcessor(disk);
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
        std::vector<Disk*>& disks = tagToDisks[obj.tag];
        LOG_DISK << "sort disk by tol reqUnit number";
        std::sort(disks.begin(), disks.end(), [=](Disk* a, Disk* b) {
            return (a->calReqUnitNum() < b->calReqUnitNum());
        });
        LOG_DISK << "assign space to disk:" << disks;
        
        for (int i = 0; i < N; i++) {
            int diskRes = 0;
            for (int k = 0; k < diskGroup[i]->disk->endToFreeSpace.size(); k++) {
                diskRes += diskGroup[i]->disk->endToFreeSpace[k].second->getResidualSize();
                LOG_DISK << "disk Res size of tagPair" <<"(" << diskGroup[i]->disk->endToFreeSpace[k].second->getTagPair().first
                    <<", "<< diskGroup[i]->disk->endToFreeSpace[k].second->getTagPair().second<<"):"
                    << diskGroup[i]->disk->endToFreeSpace[k].second->getResidualSize() << ",original size:" << diskGroup[i]->disk->endToFreeSpace[k].second->getTolSpaceSize();
            }
            LOG_DISK <<"disk Res size:"<<diskRes << ",disk req num:" << diskGroup[i]->disk->calReqUnitNum();
        }
        int j = 0;
        int repAssigned = 0;
        std::vector<int> tagIn = {};
        int tagChose = obj.tag;//开始时选择原始ta
		std::vector<int> jList = {};
        while(repAssigned < REP_NUM) {
            while (repAssigned < REP_NUM) {
                while (j<disks.size()&&(std::find(jList.begin(),jList.end(),j)!=jList.end() ||  disks[j]->getFreeSpaceSize(tagChose) < obj.size)) {
                    j++;
                }
                if (j < disks.size()) {
                    disks[j]->assignSpace(obj, static_cast<UnitOrder>(repAssigned), obj.unitOnDisk[repAssigned], tagChose);
                    obj.replica[repAssigned] = disks[j]->diskId;
		            jList.push_back(j);
                    j++; repAssigned++;
                }
                else {
                    break;
                }
            }
            if (j >= disks.size()) {
                j = 0;
                auto tagPair = disks[j]->freeSpace[tagChose].first->getTagPair();
                tagIn.push_back(tagPair.first); tagIn.push_back(tagPair.second);
                tagIn.push_back(StatisticsBucket::getMaxRltTag(tagIn.back(), tagIn));
                tagChose = tagIn.back();
            }
        }
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
    
    void simpleMultiReadStrategy();
    //如果能连读就连读，如果无法连读才进行读节点插入。
    void testMultiReadStrategy();
    void exactMultiReadStrategy();

    //如果要跳，那么就选择最大价值的object跳。需要跳的规划最后安排。
    void objectBasedReadStrategy(){
        //计算每个单元在对应磁盘上所需的时间。并且判断单元是否和之前的单元在同一磁盘上，
        LOG_DISK << "timestamp "<< Watch::getTime()<<" plan disk:";
        std::vector<std::pair<HeadPlanner*, DiskProcessor::ReqIterator>> vHeads;
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
        auto lambdaCompareHeadPlanner = [this](std::pair<HeadPlanner*, DiskProcessor::ReqIterator>& a, std::pair<HeadPlanner*, DiskProcessor::ReqIterator>& b){
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
                vHeads[i].second.toNextUnplanedReqUnit();
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
            if(obj != deletedObject){
                for (int i = 0; i < REP_NUM;i++) {
                    int diskId = obj->replica[i];
                    LOG_BplusTreeN(diskId) << "on read " << readUnit << " remove unit of " << *obj;
                }
                obj->commitUnit(unitInfo.untId, &doneRequestIds);
                this->removeObjectReqUnit(*obj, unitInfo.untId);
            }
        }
    }
    
    void excuteAllPlan() {
        for (int i = 0; i < diskGroup.size(); i++) {
            auto diskPcs = this->diskGroup[i];
            diskPcs->planner->excutePresentTimeStep(&diskPcs->handledOperations, &diskPcs->completedRead);
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

bool cmpDiskPcsByReqUnitNum(DiskProcessor*& a, DiskProcessor*& b);

#endif
