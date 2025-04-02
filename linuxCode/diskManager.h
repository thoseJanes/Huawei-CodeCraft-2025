#ifndef DISKMANAGER_H
#define DISKMANAGER_H
#include <set>
#include "disk.h"
#include "headPlanner.h"

using namespace std;

typedef BplusTree<4> BpTree;
struct ReadProfitInfo {
    int score = 0;
    int edge = 0;
    int reqNum = 0;
    int tokensCost = 0;
    int tokensEnd = 0;
};

struct ReadBlock{
    int blockLength = 0;
    int validLength = 0;
    int reqNum = 0;
    int edge = 0;
    int score = 0;
    int tokensStart = 0;
    int tokensEnd = 0;
};

class DiskProcessor{
    public:
        Disk* disk;//其中的磁头用于严格执行并返回信息。
        BpTree reqSpace;//请求单元组成的B+树
        std::array<HeadPlanner*, HEAD_NUM> planners;//优化用磁头规划器，用于临时规划
        array<vector<HeadOperator>, HEAD_NUM> handledActions;//已经开始处理的动作，作为输出到判题器的参数。
        std::vector<int> completedRead;//被头完成的读请求，作为输出到判题器的参数
        
        DiskProcessor(Disk* diskPtr):disk(diskPtr), reqSpace(){
            //为每个头创建一个planner
            for(int i=0;i<HEAD_NUM;i++){
                planners[i] = new HeadPlanner(diskPtr, diskPtr->heads[i]);
                handledActions[i] = {};
            }
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

        bool planMultiReadByReqNum(int headId);
        ReadProfitInfo getReqProfitUntilJump(int start, int startTokens, std::vector<std::pair<int, int>>& readBlocks);
        ReadBlock getMultiReadBlockAndReqNum(int reqUnit, int startTokens);


        // bool planMultiReadWithExactInfo();
        // ReadProfitInfo getExactReadProfitUntilJump(int start, int startTokens, std::vector<std::pair<int, int>>& readBlocks);
        // int getMultiReadBlockAndRelatedObjects(int reqUnit, int startTokens, int* getTokensCost, std::set<Object*>* relatedObjects, int* getReqNum);

        // bool simpleMultiRead();
        
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

    // void simpleMultiReadStrategy();
    //如果能连读就连读，如果无法连读才进行读节点插入?
    void testMultiReadStrategy();
    // void exactMultiReadStrategy();
    
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
            for(int j=0;j<HEAD_NUM;j++){
                diskPcs->planners[j]->excutePresentTimeStep(&diskPcs->handledActions[j], &diskPcs->completedRead);
            }
            
        }
    }
    //返回该回合对应磁盘磁头执行的所有动作
    std::vector<HeadOperator> getHandledOperations(int diskId, int headId){
        return std::move(diskGroup[diskId]->handledActions[headId]);
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

                for(int k=0;k<HEAD_NUM;k++){
                    diskGroup[planDiskId]->planners[k]->cancelRead(readPos);
                }
                
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
            for(int j=0;j<HEAD_NUM;j++){
                diskGroup[i]->disk->heads[j]->freshTokens();
            }
        }
    }
    const HeadPlanner* getPlanner(int i, int headId) const {
        return diskGroup[i]->planners[headId];
    }
};


#endif
