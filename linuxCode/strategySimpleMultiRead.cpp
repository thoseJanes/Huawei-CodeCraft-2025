#include "diskManager.h"

bool DiskProcessor::simpleMultiRead() {
     assert(planner->getDiskId() == this->disk->diskId);
     int readLength = 0;
     auto iter = getReqUnitIteratorUnplanedAt(planner->getLastActionNode().endPos);
     if (iter.isEnd()) { return false; };

     int startReadPos = iter.getKey(); int reqUnit = startReadPos;
     DiskUnit unitInfo = disk->getUnitInfo(reqUnit);
     //如果reqUnit还未被规划，则规划，且查看是否可以连读。
     Object* obj = sObjectsPtr[unitInfo.objId];
     while (obj != deletedObject &&//该处有对象且未被删除
         obj->unitReqNum[unitInfo.untId] > 0 && //判断是否有请求
         (!obj->isPlaned(unitInfo.untId))) {//判断是否未被规划
         readLength++;
         reqUnit = (reqUnit + 1) % disk->spaceSize; //查看下一个位置是否未被规划。
         unitInfo = disk->getUnitInfo(reqUnit);
         obj = sObjectsPtr[unitInfo.objId];
     }
     for (int i = 0; i < readLength; i++) {
         planner->appendMoveToUnplannedReadAndPlan((startReadPos + i) % disk->spaceSize);
     }
     return true;
}

bool cmpDiskPcsByReqUnitNum(DiskProcessor*& a, DiskProcessor*& b) {
    return a->reqSpace.getKeyNum() > b->reqSpace.getKeyNum();//谁当前cost最多就先规划谁。
}

void DiskManager::simpleMultiReadStrategy() {
    //计算每个单元在对应磁盘上所需的时间。并且判断单元是否和之前的单元在同一磁盘上，
    LOG_DISK << "timestamp " << Watch::getTime() << " plan disk:";
    std::vector<DiskProcessor*> diskPcsVec;
    DiskProcessor* diskPcs;
    //完成上一步行动，并且把请求指针转到当前head位置。
    for (int i = 0; i < this->diskGroup.size(); i++) {
        diskPcs = this->diskGroup[i];
        if (diskPcs->disk->head.completeAction(&diskPcs->handledOperations, &diskPcs->completedRead)) {
            LOG_DISK << "disk " << diskPcs->disk->diskId << " completeAction";
            if (Watch::toTimeStep(diskPcs->planner->getLastActionNode().endTokens) <= Watch::getTime() + PLAN_STEP - 1
                && diskPcs->reqSpace.getKeyNum() > 0) {
                diskPcsVec.push_back(diskPcs);
            }
        }
    }

    while (diskPcsVec.size()) {//loop
        //找到所有磁盘的（相对于磁头）第一个未规划plan,以及对应的object
        std::sort(diskPcsVec.begin(), diskPcsVec.end(), cmpDiskPcsByReqUnitNum);
        for (int i = 0; i < diskPcsVec.size(); i++) {
            auto diskPcs = diskPcsVec[i];
            if (Watch::toTimeStep(diskPcs->planner->getLastActionNode().endTokens) > Watch::getTime() + PLAN_STEP - 1) {
                diskPcsVec.erase(diskPcsVec.begin() + i);
                i--;
                continue;
            }
            if (!diskPcs->simpleMultiRead()) {
                diskPcsVec.erase(diskPcsVec.begin() + i);
                i--;
                continue;
            }
        }
    }
    LOG_DISK << "plan over";
    excuteAllPlan();
    LOG_DISK << "execute over";
}

