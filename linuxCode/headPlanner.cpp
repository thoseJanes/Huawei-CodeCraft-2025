#include "headPlanner.h"
#include "diskManager.h"


void HeadPlanner::appendMoveToUnplannedReadAndPlan(int unitPos) {
    auto unitInfo = this->diskPcs->disk->getUnitInfo(unitPos);
    auto obj = sObjectsPtr[unitInfo.objId];
    if (!obj->isPlaned(unitInfo.untId) && obj->isRequested(unitInfo.untId) ) {
        appendMoveTo(unitPos);
        appendAction({ READ, -1 });
        obj->planAndFreshReqSpace(unitInfo.untId, this->diskId, head->headId, Watch::toTimeStep(this->getLastActionNode().endTokens));
    }
    else {
        //appendMoveTo(unitPos);
        //appendAction({ READ, -1 });
        throw std::logic_error("this unit can not be planned!");
    }
}

void HeadPlanner::appendMoveToAllReadAndPlan(int unitPos) {
    auto unitInfo = this->diskPcs->disk->getUnitInfo(unitPos);
    if (unitInfo.objId <= 0) {//空单元
        appendMoveTo(unitPos);
        appendAction({ VREAD, -1 });
        return;
    }
    auto obj = sObjectsPtr[unitInfo.objId];
    if (!obj->isPlaned(unitInfo.untId) && obj->isRequested(unitInfo.untId)) {
        if(Watch::getTime() == 1355 && unitPos == 4685 && this->diskId==4){
            int test = 0;
        }
        appendMoveTo(unitPos);
        appendAction({ READ, -1 });
        int planTimeStep = Watch::toTimeStep(this->getLastActionNode().endTokens);
        // 是否要在这时判断过期呢？值不值？
        // auto earliestReq = obj->objRequests.front();
        // auto reqId = obj->objRequests.begin();
        // while(planTimeStep > (*reqId)->createdTime + EXTRA_TIME - 1 && reqId != obj->objRequests.end()){
        //     if(((*reqId)->unitFlags&(1<<unitInfo.untId))==0){
        //         diskPcs->busyReqId.push_back((*reqId)->reqId);
        //         reqId ++;
        //     }
        // }
        obj->planAndFreshReqSpace(unitInfo.untId, this->diskId, head->headId, planTimeStep);
    }
    else {
        appendMoveTo(unitPos);
        appendAction({ VREAD, -1 });
        //appendMoveTo(unitPos);
        //appendAction({ VREAD, -1 });
    }
}

void HeadPlanner::freshVRead(){//如果存在VRead被请求，则将其转换为Read并进行plan
    for(auto it = actionNodes.begin();it != actionNodes.end();it++){
        if((*it).action.action == VREAD){
            auto unitInfo = diskPcs->disk->getUnitInfo(((*it).endPos-1+spaceSize)%spaceSize);
            if(unitInfo.objId>0 && 
                    sObjectsPtr[unitInfo.objId]->unitReqNum[unitInfo.untId]>0
                    && (!sObjectsPtr[unitInfo.objId]->isPlaned(unitInfo.untId))){
                sObjectsPtr[unitInfo.objId]->planAndFreshReqSpace(unitInfo.untId, diskId, head->headId, Watch::toTimeStep((*it).endTokens));
                (*it).action.action = READ;
            }
        }
    }
}

void HeadPlanner::cancelAllAction(){
        for(auto it = std::next(actionNodes.begin());it!=actionNodes.end();it++){
            auto unitInfo = diskPcs->disk->getUnitInfo(((*it).endPos-1)%spaceSize);
            auto obj = sObjectsPtr[unitInfo.objId];
            if((*it).action.action == READ && obj->isRequested(unitInfo.untId)){
                for(int i=0;i<REP_NUM;i++){
                    obj->reqSpaces[i]->insert(obj->unitOnDisk[i][unitInfo.untId], nullptr);
                    obj->clearPlaned(unitInfo.untId);
                }
            }
        }
        actionNodes.erase(std::next(actionNodes.begin()),actionNodes.end());
    }


void HeadPlanner::insertUnplannedReadAsBranch(int unitPos, int* getReadOverTokens, int* getScoreLoss) {
    LOG_PLANNERN(diskId) << "\ninserting read on " << unitPos << "as new branch";
    int pstDist;
    auto aftIt = getShortestDistance(unitPos, &pstDist);
    LOG_PLANNERN(diskId) << "shortest distance node " << *aftIt << " in "<< actionNodes;
    HeadPlanner* branch = new HeadPlanner(diskPcs, head, *(aftIt));
    readBranches.insert({ unitPos, {aftIt, branch} });
    branch->appendMoveTo(unitPos);
    branch->appendAction({ READ, -1 });
    *getReadOverTokens = branch->getLastActionNode().endTokens;
    auto test_lastPos = (*aftIt).action.action;
    aftIt++;
    
    if (aftIt != this->actionNodes.end()) {
        auto nextAction = (*aftIt).action.action;
        if (nextAction == PASS || nextAction == JUMP) {
            branch->appendMoveTo((*aftIt).endPos);
            aftIt++;
        }
        else if ((nextAction == READ || nextAction == VREAD) 
                && (test_lastPos == READ || test_lastPos == VREAD)) {
            throw std::logic_error("duplicate read plan");
            //throw std::logic_error("error!!");
            //如果是READ，那么上一个endPos距离unitPos最近且为下一个位置减1,那么上一个endPos就是uniPos。
            //assert(branch->actionNodes.front().endPos == unitPos);
            //但是它既然停在了上一个位置，又没有读。就很奇怪了。不读为什么要停在那。
            //throw std::logic_error("strange!!!");
            //因为规划不是按顺序来的。
        }//如果是JUMP，直接appendAction即可。
    }

    std::vector<Object*> relatedObjects = {};
    while (aftIt != this->actionNodes.end()) {
        branch->appendAction((*aftIt).action, true);
        const ActionNode& newNode = branch->getLastActionNode();
        if (getScoreLoss != nullptr && newNode.action.action == READ) {//规划，VREAD不需要规划。
            auto unitInfo = diskPcs->disk->getUnitInfo((newNode.endPos - 1 + spaceSize)%spaceSize);
            Object* obj = sObjectsPtr[unitInfo.objId];
            obj->virBranchPlan(unitInfo.untId, Watch::toTimeStep(newNode.endTokens), false);
            relatedObjects.push_back(obj);
        }
        aftIt++;
    }
    LOG_PLANNERN(diskId) << "generate branch " << branch->actionNodes;
    if (getScoreLoss != nullptr) {
        for (int i = 0; i < relatedObjects.size(); i++) {
            *getScoreLoss += relatedObjects[i]->getScoreLoss();
        }
    }
    //向后移动看看能不能改进总价值?
}

void HeadPlanner::mergeUnplannedReadBranch(int unitPos) {
    LOG_PLANNERN(diskId) << "\nmerge branch on reading "<<unitPos;
    auto branchInfo = this->readBranches.at(unitPos);
    actionNodes.erase(std::next(branchInfo.first, 1), actionNodes.end());//删除branchInfo.first之后的节点。
    actionNodes.splice(actionNodes.end(), branchInfo.second->actionNodes,
        std::next(branchInfo.second->actionNodes.begin(), 1), branchInfo.second->actionNodes.end());//把branch的分支移动过来。
    branchInfo.first++;//移动到新的拼接部分的节点上。
    while (branchInfo.first != this->actionNodes.end()) {//规划读操作
        LOG_PLANNERN(diskId) << "on node " << *branchInfo.first;
        if ((*branchInfo.first).action.action == READ) {
            int newPlanedTime = Watch::toTimeStep((*branchInfo.first).endTokens);
            int readPos = ((*branchInfo.first).endPos - 1+ spaceSize)%spaceSize;
            auto unitInfo = diskPcs->disk->getUnitInfo(readPos);
            LOG_PLANNER << "plan unit " << unitInfo.untId<<" for obj "<<unitInfo.objId<< ", time:"<<newPlanedTime<<" on disk "<<diskId;
            sObjectsPtr[unitInfo.objId]->planAndFreshReqSpace(unitInfo.untId, this->diskId, head->headId, newPlanedTime);
        }
        branchInfo.first++;
    }

    this->readBranches.erase(unitPos);
    for (auto it = this->readBranches.begin(); it != this->readBranches.end(); it++) {
        delete (*it).second.second;//删除除了当前分支外的所有分支
    }
    this->readBranches.clear();

    this->readBranches = std::move(branchInfo.second->readBranches);//把branch的分支移动到当前。
    delete branchInfo.second;
}

void HeadPlanner::dropReadBranch(int unitPos) {
    LOG_PLANNERN(diskId) << "\ndrop branch on reading " << unitPos;
    auto branchInfo = this->readBranches.at(unitPos);
    this->readBranches.erase(unitPos);
    delete branchInfo.second;
}


const Disk* HeadPlanner::getDisk() const { return this->diskPcs->disk; }


