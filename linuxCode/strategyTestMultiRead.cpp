#include "diskManager.h"
#include "readBlock.h"

//判断是否能够连读。如果能，则连读。
//该函数可以找到不同位置开始的较长的连读段。适合jump时使用。
bool DiskProcessor::planMultiReadByReqNum(int headId) {//每次选择一个请求单元最多的开始规划？
    auto planner = this->planners[headId];
    int searchNum = 0;
    auto iter = getReqUnitIteratorUnplanedAt(planner->getLastActionNode().endPos);
    if (iter.isEnd()) { return false; }
    vector<pair<int, int>> readBlocks = {};

    int startTokens = planner->getLastActionNode().endTokens;
    if (getDistance(iter.getKey(), planner->getLastActionNode().endPos, disk->spaceSize) >= G) {
        int reqUnit = iter.getKey();//第一个请求位置
        double tempProfit = 0; double maxProfit = 0; 

        vector<vector<pair<int, int>>> allReadBlocks = {};
        vector<ReadProfitInfo> allInfo = {};
        ReadProfitInfo maxInfo;
        while (++searchNum <= MULTIREAD_SEARCH_NUM) {//对于跳读而言，找到一个最长的读，然后从此处开始判断是否连读。
            vector<pair<int, int>> tempReadBlocks = {};
            auto info = getReqProfitUntilJump(reqUnit, startTokens, tempReadBlocks);
            tempProfit = info.edge*1.0;// / (info.tokensEnd - startTokens);
            int nextStart = calNextStart(tempReadBlocks);

            allReadBlocks.push_back(std::move(tempReadBlocks));
            allInfo.push_back(info);

            if(maxProfit < tempProfit){
                readBlocks = allReadBlocks.back();
                maxProfit = tempProfit;
                maxInfo = info;
            }
            iter.toUnplanedNoLessThan(nextStart);
            if (iter.isEnd()) { break; }// throw std::logic_error("既然已经进入了循环，iter就不会到end"); 
            reqUnit = iter.getKey();//获取下一个未规划的单元。
        }
    }
    else {//判断是继续读还是直接跳。
        int passTokens = getDistance(iter.getKey(), planner->getLastActionNode().endPos, disk->spaceSize);
        int passStartTokens = calNewTokensCost(startTokens, passTokens, true);
        vector<pair<int, int>> tempReadBlocks = {};
        auto passProfitInfo = getReqProfitUntilJump(iter.getKey(), passStartTokens, readBlocks);
        
        int nextReqUnit = readBlocks.back().first + readBlocks.back().second;
        iter = getReqUnitIteratorUnplanedAt(nextReqUnit);
        //检查跳是否能取得更高收益。
        int reqUnit = iter.getKey();
        int maxProfit = 0;
        int jumpStartTokens = calNewTokensCost(startTokens, G, false);
        while (++searchNum <= PASS_TO_JUMP_SEARCH_NUM) {//对于跳读而言，找到一个最长的读，然后从此处开始判断是否连读。
            std::vector<std::pair<int, int>> tempReadBlocks = {};
            auto info = getReqProfitUntilJump(reqUnit, jumpStartTokens, tempReadBlocks);
            int passLoss = passProfitInfo.edge * (Watch::toTimeStep(passStartTokens)-Watch::getTime()) + info.edge * (Watch::toTimeStep(passProfitInfo.tokensEnd)+1-Watch::getTime());
            int jumpLoss = info.edge * (Watch::toTimeStep(jumpStartTokens)-Watch::getTime()) + passProfitInfo.edge * (Watch::toTimeStep(info.tokensEnd)+1-Watch::getTime());
            // int passLoss = passProfitInfo.reqNum * passTokens + info.reqNum * (passProfitInfo.tokensCost + passTokens + G);
            // int jumpLoss = info.reqNum * G + passProfitInfo.reqNum * (G + info.tokensCost + G);

            int nextStart = calNextStart(tempReadBlocks);
            if (passLoss - jumpLoss > maxProfit) {
                maxProfit = passLoss - jumpLoss;
                readBlocks = std::move(tempReadBlocks);
                //改为break之后分数略微降低，时间大概快了3秒。//break;//因为这种情况比较罕见，且防止切换到的对象的最后一步为read。不如，如果最后一步为read就直接切换？
            }
            iter.toUnplanedNoLessThan(nextStart);
            if (iter.isEnd()) { break; }// throw std::logic_error("既然已经进入了循环，iter就不会到end"); 
            reqUnit = iter.getKey();//获取下一个未规划的单元。
        }
    }
            
    planner->appendMoveTo(readBlocks.front().first);
    //LOG_PLANNERN(planner->getDiskId()) << "\nbefore add, planner:" << planner;
    //LOG_PLANNER << "planner " << this->disk->diskId << " " << planner->getDiskId() <<" planning ";
    for (int k = 0; k < readBlocks.size(); k++) {
        int start = readBlocks[k].first;
        for (int j = 0; j < readBlocks[k].second; j++) {
            planner->appendMoveToAllReadAndPlan((start + j) % disk->spaceSize);
            LOG_PLANNER  << " plan for unit "
                << (start + j) % disk->spaceSize;
            LOG_PLANNERN(planner->getDiskId()) << " plan for unit "
                << (start + j) % disk->spaceSize;
        }
        //但是只把当前时间步相关的行动入栈并等待执行。
        if (Watch::toTimeStep(planner->getLastActionNode().endTokens) > Watch::getTime()) {
            //以readBlock为单元加入读。
        }
        // else {//把刚分配的部分保护起来。
        //     for (int j = 0; j < readBlocks[k].second; j++) {
        //         auto info = disk->getUnitInfo((start + j) % disk->spaceSize);
        //         auto obj = sObjectsPtr[info.objId];
        //         if (info.objId > 0) {
        //             //obj->plan(info.untId, disk->diskId, headId);//为当前时间步规划。防止其它磁盘也用到该磁盘现在规划的单元。
        //             LOG_OBJECT << "obj " << obj->objId << " plan for unit "
        //                 << info.untId << " in pos " << (start + j) % disk->spaceSize << " on disk " << this->disk->diskId;
        //             LOG_PLANNER << "obj " << obj->objId << " plan for unit "
        //                 << info.untId << " in pos " << (start + j) % disk->spaceSize << " on disk " << this->disk->diskId;
        //         }
        //     }
        // }
    }//也可以直接执行，然后清除未执行完的内容，防止plan的麻烦。
    //LOG_PLANNERN(planner->getDiskId()) << "after add, planner:" << planner;
    return true;
}

int DiskProcessor::calNextStart(std::vector<std::pair<int, int>>& readBlocks) {
    //return (readBlocks.back().first + readBlocks.back().second) % disk->spaceSize;

    if (readBlocks.size() <= 1) {
        return (readBlocks.front().first + readBlocks.front().second) % this->disk->spaceSize;
    }
    else {
        int maxPassLen = 0; int maxPassLenStart = 0; int maxReadLen = readBlocks[0].second;
        for (int i = 0; i < readBlocks.size()-1; i++) {
            int passLen = readBlocks[i + 1].first - readBlocks[i].first - readBlocks[i].second;
            if (passLen > maxPassLen) {
                maxPassLen = passLen;
                maxPassLenStart = (readBlocks[i].first + readBlocks[i].second)%disk->spaceSize;
            }
            if (readBlocks[i+1].second < maxReadLen) {
                maxReadLen = readBlocks[i+1].second;
            }
        }
        if (maxReadLen < maxPassLen) {
            return maxPassLenStart;
        }
        else {
            return (readBlocks.back().first + readBlocks.back().second) % disk->spaceSize;
        }
    }
}

ReadProfitInfo DiskProcessor::getReqProfitUntilJump(int start, int startTokens, std::vector<std::pair<int, int>>& readBlocks) {
    auto iter = getReqUnitIteratorUnplanedAt(start);
    if (iter.isEnd()) { return { 0, 1 }; }
    int cost = getDistance(iter.getKey(), start, disk->spaceSize); assert(cost == 0);
    int tolReadLen = 0; int tolLen = 0;
    ReadProfitInfo readProfit; readProfit.tokensEnd = startTokens;
    int startStep = Watch::toTimeStep(startTokens);
    while (!iter.isEnd() && getDistance(iter.getKey(), start, disk->spaceSize) < G &&
        Watch::toTimeStep(readProfit.tokensEnd) <= startStep + 1){ //&& Watch::getTime()>32000) || 
        //(tolLen < 20 && Watch::getTime()<=32000)) ) {//查看一定步之内有多少收益。如果读数量太多可能会超出。
        int passDist = getDistance(iter.getKey(), start, disk->spaceSize);
        readProfit.tokensEnd = calNewTokensCost(readProfit.tokensEnd, passDist, true);

        start = iter.getKey();
        auto readBlock = getMultiReadBlockAndReqNum(iter.getKey(), readProfit.tokensEnd);
        readProfit.reqNum += readBlock.reqNum;
        #ifdef ENABLE_OBJECTSCORE
        readProfit.score += readBlock.score;
        #endif
        readProfit.edge += readBlock.edge;
        readProfit.tokensEnd = readBlock.tokensEnd;

        tolReadLen += readBlock.blockLength; 
        tolLen += (readBlock.blockLength + passDist);
        readBlocks.push_back({ start, readBlock.blockLength });

        start = (start + readBlock.blockLength) % disk->spaceSize;
        //LOG_PLANNER << "block end:" << start;
        iter.toUnplanedNoLessThan(start);
        //iter = getReqUnitIteratorUnplanedAt(start);//会找到下一个或者和start相等的。
    }
    //LOG_PLANNER << "read blocks size:" << readBlocks.size();
    return readProfit;
}

//profit用请求数来计算？还是用分数来计算？先用请求数试试。
//返回{ tolReqNum , tolTokensCost }。计算了移动到第一个起始请求的花费。
ReadBlock DiskProcessor::getMultiReadBlockAndReqNum(int reqUnit, int startTokens) {
    int tempPos = reqUnit;
    ReadBlock readBlock;
    ReadBlock tempBlock;
    tempBlock.tokensEnd = startTokens;
    
    int multiReadTokensProfit = 0;
    int multiReadLen = 0;
    int invalidReadLen = 0;
    DiskUnit unitInfo = this->disk->getUnitInfo(tempPos);
    Object* obj = sObjectsPtr[unitInfo.objId];
    while (invalidReadLen < MULTIREAD_JUDGE_LENGTH) {//从tempPos开始，是否可以读原本不需要读的块来获取收益。
        if (unitInfo.objId>0 && obj != deletedObject && obj != nullptr &&//该处有对象且未被删除
            obj->unitReqNum[unitInfo.untId] > 0 && //判断是否有请求
            (!obj->isPlaned(unitInfo.untId))) {
            
            multiReadTokensProfit = std::min(0,
                multiReadTokensProfit
                + getReadConsumeAfterN(multiReadLen)
                - getReadConsumeAfterN(tempBlock.blockLength));
            multiReadLen++;
            invalidReadLen = 0;

            tempBlock.tokensEnd = calNewTokensCost(tempBlock.tokensEnd, getReadConsumeAfterN(tempBlock.blockLength), false);
            tempBlock.blockLength++;
            tempBlock.reqNum += obj->unitReqNum[unitInfo.untId];
            tempBlock.validLength++;
            #ifdef ENABLE_OBJECTSCORE
            obj->calUnitScoreAndEdge(unitInfo.untId, &tempBlock.score, &tempBlock.edge);
            #else
            int temp;
            obj->calUnitScoreAndEdge(unitInfo.untId, &temp, &tempBlock.edge);
            #endif
        }
        else {
            
            multiReadTokensProfit =
                multiReadTokensProfit
                - getReadConsumeAfterN(tempBlock.blockLength) + 1;
            invalidReadLen++;
            multiReadLen = 0;

            tempBlock.tokensEnd = calNewTokensCost(tempBlock.tokensEnd, getReadConsumeAfterN(tempBlock.blockLength), false);
            tempBlock.blockLength++;
        }
        if (multiReadTokensProfit >= 0) {//没有连读损耗
            // if(tempBlock.tokensEnd - startTokens > G*6){
            //     return readBlock;
            // }//应该直接返回，这样获取的就是上一个readBlock
            readBlock = tempBlock;
            
        }
        tempPos = (tempPos + 1) % this->disk->spaceSize; //查看下一个位置是否未被规划。
        unitInfo = disk->getUnitInfo(tempPos);
        obj = sObjectsPtr[unitInfo.objId];
    }
            
    return readBlock;
}

//优化方向：修改成未规划的键数量。
bool cmpDiskPcsByReqUnitNum(pair<DiskProcessor*, int>& a, pair<DiskProcessor*, int>& b){
    return a.first->reqSpace.getKeyNum() > b.first->reqSpace.getKeyNum();//谁当前cost最多就先规划谁。
}

void DiskManager::testMultiReadStrategy() {
    //计算每个单元在对应磁盘上所需的时间。并且判断单元是否和之前的单元在同一磁盘上，
    //LOG_DISK << "timestamp " << Watch::getTime() << " plan disk:";
    std::vector<std::pair<DiskProcessor*, int>> diskPcsVec;
    DiskProcessor* diskPcs;
    //完成上一步行动，并且把请求指针转到当前head位置。
    for (int i = 0; i < this->diskGroup.size(); i++) {
        diskPcs = this->diskGroup[i];
        for(int j=0;j<HEAD_NUM;j++){
            if (diskPcs->disk->heads[j]->completeAction(&diskPcs->handledActions[j], &diskPcs->completedRead)) {
                assert(diskPcs->disk->heads[j]->presentTokens == G);
                //LOG_DISK << "disk " << diskPcs->disk->diskId << " completeAction";
                if (Watch::toTimeStep(diskPcs->planners[j]->getLastActionNode().endTokens) <= Watch::getTime() + PLAN_STEP - 1
                        && diskPcs->reqSpace.getKeyNum() > 0) {
                    diskPcsVec.push_back({diskPcs, j});
                }else{
                    diskPcs->planners[j]->freshVRead();
                }
            }
        }
    }

    while (diskPcsVec.size()) {//loop
        //找到所有磁盘的（相对于磁头）第一个未规划plan,以及对应的object
        std::sort(diskPcsVec.begin(), diskPcsVec.end(), cmpDiskPcsByReqUnitNum);
        for (int i = 0; i < diskPcsVec.size(); i++) {
            auto diskPcs = diskPcsVec[i].first;
            auto headId = diskPcsVec[i].second;
            if (Watch::toTimeStep(diskPcs->planners[headId]->getLastActionNode().endTokens) > Watch::getTime() + PLAN_STEP - 1) {
                diskPcsVec.erase(diskPcsVec.begin() + i);
                i--;
                continue;
            }
            if (!diskPcs->planMultiReadByReqNum(headId)) {
                diskPcsVec.erase(diskPcsVec.begin() + i);
                i--;
                continue;
            }
        }
    }
    //LOG_DISK << "plan over";
}


