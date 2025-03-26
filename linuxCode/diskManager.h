#ifndef DISKMANAGER_H
#define DISKMANAGER_H
#include "headPlanner.h"


class DiskProcessor{
    public:
        Disk* disk;//其中的磁头用于严格执行并返回信息。

        BplusTree<4> reqSpace;//请求单元组成的B+树
        HeadPlanner planner;//优化用磁头规划器，用于临时规划
        std::vector<HeadOperator> handledOperations = {};//已经开始处理的动作，作为输出到判题器的参数。
        std::vector<int> completedRead = {};//被头完成的读请求，作为输出到判题器的参数
        
        DiskProcessor(Disk* diskPtr):disk(diskPtr), reqSpace(), planner(disk){
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
        // //判断是否能够连读。如果能，则连读。
        // void multiRead(HeadPlanner& headPlanner){
        //     int times = 0;
        //     int reqUnit = headPlanner.getLastHeadPos();
        //     LOG_ACTIONSN(headPlanner.diskId) << "test multiRead from " << reqUnit;
        //     DiskUnit unitInfo = this->disk->getUnitInfo(reqUnit);
        //     //如果reqUnit还未被规划，则规划，且查看是否可以连读。
        //     Object* obj = sObjectsPtr[unitInfo.objId];
        //     while(obj != &deletedObject &&//该处有对象且未被删除
        //                 obj->unitReqNum[unitInfo.untId]>0&& //判断是否有请求
        //                     (!obj->isPlaned(unitInfo.untId))){//判断是否未被规划
        //         LOG_DISK << "hold read on unit "<< 
        //             reqUnit<<" from obj " << unitInfo.objId << " unitId " << unitInfo.untId;
        //         //持续一回合的plan。commitPlan中会有持续多个回合的plan
        //         obj->plan(unitInfo.untId, this->disk->diskId);
        //         times ++;
        //         headPlanner.freshActionsInfo({ READ, 1 });
        //         if (headPlanner.getLastTokenCost() > 2 * G) { break; }//超过两回合就退出。
        //         reqUnit = (reqUnit+1)%this->disk->spaceSize; //查看下一个位置是否未被规划。
        //         unitInfo = this->disk->getUnitInfo(reqUnit);
        //         obj = sObjectsPtr[unitInfo.objId];
        //     }
        //     if (times == 0) {
        //         throw std::logic_error("error! times equals 0. this unit should has not been planed! ");
        //     }
        // }

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
                LOG_BplusTreeN(obj.replica[i]) << *diskGroup[obj.replica[i]]->reqSpace.root;
                LOG_BplusTreeN(obj.replica[i]) << "\n(num:" << diskGroup[obj.replica[i]]->reqSpace.keyNum << ")";

                LOG_BplusTreeN(obj.replica[i]) << "over insert unit of obj " << obj
                     <<" tree:" << diskGroup[obj.replica[i]]->reqSpace;
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
    

    //该函数只运行一次，是全局性的规划。
    
    //如果要跳，那么就选择最大价值的object跳。需要跳的规划最后安排。
    void planDisksRead(){
        //计算每个单元在对应磁盘上所需的时间。并且判断单元是否和之前的单元在同一磁盘上，
        LOG_DISK << "timestamp "<< Watch::getTime()<<" plan disk:";
        std::vector<HeadPlanner*> vHeads;
        std::vector<HeadPlanner*> jumpVHeads;
        DiskProcessor* diskPcs; Disk* disk;
        //完成上一步行动，并且把请求指针转到当前head位置。
        for(int i=0;i<this->diskGroup.size();i++){
            diskPcs = this->diskGroup[i];
            disk = diskPcs->disk;
            if(disk->head.completeAction(&diskPcs->handledOperations, &diskPcs->completedRead)){
                LOG_DISK << "disk "<< disk->diskId<<" completeAction";
                if(diskPcs->roundReqUnitToPos(disk->head.headPos)){
                    LOG_ACTIONSN(disk->diskId) << "round disk to head pos "<<disk->head.headPos
                                                <<", present anchor pos "
                                                <<diskPcs->reqSpace.getAnchor().startKey
                                                <<diskPcs->reqSpace.getNextKeyByAnchor(false);
                    vHeads.push_back(&diskPcs->planner);
                }
                else {
                    LOG_ACTIONSN(i) << "disk "<<i<<" has no reqUnit";
                }
            }
        }
        
        //先规划工期短的磁盘。如果工期都超过当前3个时间步，则先规划请求数少的磁盘。(或者先规划空行动最多的磁盘)
        auto lambdaCompare = [this](HeadPlanner*& a, HeadPlanner*& b){
            if(a->getLastActionNode().endTokens/G > Watch::getTime() + 2){
                if(b->getLastActionNode().endTokens/G > Watch::getTime() + 2){
                    return diskGroup[a->getDiskId()]->reqSpace.getKeyNum() > 
                            diskGroup[b->getDiskId()]->reqSpace.getKeyNum();//谁keyNum最少就先规划谁。
                }else{
                    return true;//把b放后面
                }
            }
            return a->getLastActionNode().endTokens < b->getLastActionNode().endTokens;//谁当前cost最多就先规划谁。
        };
        std::sort(vHeads.begin(), vHeads.end(), lambdaCompare);
        
        //找到所有磁盘的第一个未规划plan。

        //找到可达plan对应的对象。
        //先规划价值最大的对象。
        while(vHeads.size() && vHeads.back()->getLastTokenCost() <G) {//规划一个回合?还是完全规划然后优化？
            auto vHead = vHeads.back();
            diskPcs = this->diskGroup[vHead->diskId];
            disk = diskPcs->disk;
            //LOG_DISK << "present head:" << *vHead;
            int unitPos = diskPcs->getNextUnplanedReqUnit();
            LOG_DISK << "present head:" << *vHead;
            LOG_DISK << "next unit pos:" << unitPos;
            if(unitPos>=0){
                auto unitInfo = disk->getUnitInfo(unitPos);
                LOG_ACTIONSN(vHead->diskId) << "plan disk to unplaned reqUnit, [pos(" 
                    << unitPos << "):obj(" << unitInfo.objId << "):unt(" << unitInfo.untId << ")";
                LOG_ACTIONSN(vHead->diskId) << "obj info:" << *sObjectsPtr[unitInfo.objId];
                if (diskPcs->toReqUnit(*vHead, unitPos)) {//只有当前时间帧能到达目标位置才能在目标位置开始读。
                    diskPcs->multiRead(*vHead);
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

    void planObjectsRead(){
        auto lambdaCompare = [this](Object*& a, Object*& b){
            return objectScore(a) > objectScore(b);
            //sort会把true的放前面。也就是把分数大的放在前面。
        };
        std::sort(requestedObjects.begin(), requestedObjects.end(), lambdaCompare);
        //规划大分对象的磁盘，考虑因素：能否最快地完成这个对象。
        for(auto it = requestedObjects.begin();it!=requestedObjects.end();it++){
            Object* obj = *it;
            int earliest = Watch::getTime();//这个earliest表示的是前面单元最快完成时间步。
            for(int i=0;i<obj->size;i++){//以请求单元上累积的请求数从大到小的顺序规划.小者会被大者阻塞。
                if(obj->unitReqNum[obj->unitReqNumOrder[i]]>0){//或者if(!obj->isPlanned())，来选择未规划的单元。
                    planUnitsRead(obj, obj->unitReqNumOrder, i, &earliest);
                }
            }
        }
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
        int diskSelected = 0;
        int unitId = unitReqNumOrder[place];
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj->replica[i];
            auto actionPlanner = diskGroup[diskId]->planner;
            actionPlanner.insertReadAsBranch(obj->unitOnDisk[i][unitId], readOverTokens+i, &scoreLoss);
            tolTokens[i] = actionPlanner.getLastActionNode().endTokens;
            scoreGain = std::min<int>(*aheadEarliest - readOverTokens[i]/G, 0) * obj->edgeValue;
            assert(scoreGain <= 0);
            tolScore[i] = scoreGain - scoreLoss;//为负数。
            assert(tolScore[i] <= 0);
            //记录分数最大的副本（优化方向：可以改为规划最长长度(有上限)和分数加权为总分数）
            if(i=0){
                diskSelected = 0;
                maxTolScore = tolScore[0];
            }else if(tolScore[i]>maxTolScore){
                maxTolScore = tolScore[i];
                diskSelected = i;
            }else if(tolScore[i] == maxTolScore && tolTokens[i] < tolTokens[diskSelected]){
                //如果分数相等，则选择目前规划长度更小的那个规划。
                maxTolScore = tolScore[i];
                diskSelected = i;
            }
        }

        //简单地选择这一单元规划后分数最大的。
        *aheadEarliest = std::max<int>(*aheadEarliest, readOverTokens[diskSelected]/G);
        for(int i=0;i<REP_NUM;i++){
            int unitPos = obj->unitOnDisk[i][unitId];
            int diskId = obj->replica[i];
            auto actionPlanner = diskGroup[diskId]->planner;
            if(i==diskSelected){
                actionPlanner.mergeReadBranch(unitPos);
            }else{
                actionPlanner.dropReadBranch(unitPos);
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
        LOG_DISK << "test commit plan for disk "<< diskId <<"\nplan:"<<actionsPlan;
        if(disk->head.completeAction(&diskPcs->handledOperations, &diskPcs->completedRead)){
            int planNum = actionsPlan.size();
            for(auto it=std::next(actionsPlan.begin(),1);it!=actionsPlan.end();it++){
                LOG_DISK << "loop continue";
                auto actionNode = *it;
                if(!disk->head.beginAction(actionNode.action)){
                    LOG_DISK << "begin fail, some actions to be completed";
                    break;//要么是之前行动没有做完，要么是输入参数出了问题。
                }else{
                    if(actionNode.action.action == READ){
                        int actionFinishStep = actionNode.endTokens/G;
                        int pos = actionNode.endPos - 1;//因为读操作现在只可能是单步的。
                        
                        auto unitInfo = disk->getUnitInfo(pos);
                        sObjectsPtr[unitInfo.objId]->plan(unitInfo.untId, diskId, actionFinishStep, false);
                    }
                    LOG_DISK << "disk "<<disk->diskId<<" begin action:"<<actionNode;
                    if(!disk->head.completeAction(&diskPcs->handledOperations, &diskPcs->completedRead)){
                        break;//剩下的时间片不足以完成这个行动
                    }
                    assert((actionNode.endTokens-1)/G == Watch::getTime());//时间对齐。
                    LOG_DISK << "disk "<<disk->diskId<<" complete action:"<<actionNode;
                    //剩下的时间片完成了这个行动。继续迭代以喂入下一个活动。
                }
            }
        }
        headPlanner->diskId = -1;
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
    std::list<HeadOperator> getHandledOperations(int diskId){
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
    const HeadPlanner* getPlanner(int i) const {
        return &diskGroup[i]->planner;
    }
};

#endif