#include "headPlanner.h"
#include "diskManager.h"


void HeadPlanner::appendMoveToUnplannedReadAndPlan(int unitPos) {
    auto unitInfo = this->diskPcs->disk->getUnitInfo(unitPos);
    auto obj = sObjectsPtr[unitInfo.objId];
    if (!obj->isPlaned(unitInfo.untId) && obj->isRequested(unitInfo.untId) ) {
        appendMoveTo(unitPos);
        appendAction({ READ, -1 });
        obj->plan(unitInfo.untId, this->diskId, head->headId, this->getLastActionNode().endTokens, false);
    }
    else {
        //appendMoveTo(unitPos);
        //appendAction({ READ, -1 });
        throw std::logic_error("this unit can not be planned!");
    }
}

void HeadPlanner::appendMoveToAllReadAndPlan(int unitPos) {
    auto unitInfo = this->diskPcs->disk->getUnitInfo(unitPos);
    if (unitInfo.objId < 0) {//空单元
        appendMoveTo(unitPos);
        appendAction({ VREAD, -1 });
        return;
    }
    auto obj = sObjectsPtr[unitInfo.objId];
    if (obj!=deletedObject && !obj->isPlaned(unitInfo.untId) && obj->isRequested(unitInfo.untId)) {
        appendMoveTo(unitPos);
        appendAction({ READ, -1 });
        obj->plan(unitInfo.untId, this->diskId, head->headId, Watch::toTimeStep(this->getLastActionNode().endTokens), false);
        
        for(int i=0;i<REP_NUM;i++){
            int diskId = obj->replica[i];
            int unitPos = obj->unitOnDisk[i][unitInfo.untId];
            diskPcs->reqSpace.remove(unitPos);
        }
    }
    else {
        appendMoveTo(unitPos);
        appendAction({ VREAD, -1 });
        //appendMoveTo(unitPos);
        //appendAction({ VREAD, -1 });
    }
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
            sObjectsPtr[unitInfo.objId]->plan(unitInfo.untId, this->diskId, head->headId, newPlanedTime, false);
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


