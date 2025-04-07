#ifndef STRATEGY_H_
#define STRATEGY_H_
#include "diskManager.h"
#include "readBlock.h"

#define SCORE_RECORD_LEN 20
class Strategy{
public:
    list<int**> scoreRecord;
    vector<DiskProcessor*>& diskGroup;
    bool** toBePlanned;
    Strategy(int plannerNum, int headNum, vector<DiskProcessor*>& diskGroup)
                : diskGroup(diskGroup){
        int* recoreBuffer = (int*)malloc(plannerNum*headNum*sizeof(int)*SCORE_RECORD_LEN);
        memset(recoreBuffer, 0, plannerNum*headNum*sizeof(int)*SCORE_RECORD_LEN);
        int** toRecord = (int**)malloc(plannerNum*SCORE_RECORD_LEN*sizeof(int*));
        for(int i=0;i<SCORE_RECORD_LEN;i++){
            for(int j=0;j<plannerNum;j++){
                toRecord[i*plannerNum+j] = recoreBuffer + headNum*(i*plannerNum+j);
            }
            scoreRecord.push_back(toRecord + i*plannerNum);
        }

        bool* toBePlannedBuffer = (bool*)malloc(plannerNum*headNum*sizeof(bool));
        memset(toBePlannedBuffer, false, plannerNum*headNum*sizeof(bool));
        toBePlanned = (bool**)malloc(plannerNum*sizeof(bool*));
        for(int j=0;j<plannerNum;j++){
            toBePlanned[j] = toBePlannedBuffer + headNum*j;
        }
    }
    void multiReadStrategy() {
        //计算每个单元在对应磁盘上所需的时间。并且判断单元是否和之前的单元在同一磁盘上，
        //LOG_DISK << "timestamp " << Watch::getTime() << " plan disk:";
        vector<DiskProcessor*> diskPcsVec;
        DiskProcessor* diskPcs;
        //完成上一步行动，并且把请求指针转到当前head位置。
        for (int i = 0; i < this->diskGroup.size(); i++) {
            diskPcs = this->diskGroup[i];
            for(int j=0;j<HEAD_NUM;j++){
                assert(diskPcs->disk->heads[j]->completeAction(&diskPcs->handledActions[j], &diskPcs->completedRead));
                assert(diskPcs->disk->heads[j]->presentTokens == G);
                //LOG_DISK << "disk " << diskPcs->disk->diskId << " completeAction";
                if (Watch::toTimeStep(diskPcs->planners[j]->getLastActionNode().endTokens) <= Watch::getTime() + PLAN_STEP - 1
                        && diskPcs->reqSpace.getKeyNum() > 0) {
                    //处于规划时间内且有对象可规划。
                    toBePlanned[i][j] = true;
                }else{
                    toBePlanned[i][j] = false;
                }
            }
        }

        //按某种方式排序
        std::sort(diskPcsVec.begin(), diskPcsVec.end(), cmpDiskPcsByReqUnitNum);

        for (int i = 0; i < diskPcsVec.size(); i++) {
            auto diskPcs = diskPcsVec[i];
            planDisk(diskPcs);
        }
        //LOG_DISK << "plan over";
    }
    bool planDisk(DiskProcessor* diskPcs);
};


struct ReadBlockInfo{
    vector<pair<int,int>> blocks;
    ReadProfitInfo profit;
}
//判断是否能够连读。如果能，则连读。
//该函数可以找到不同位置开始的较长的连读段。适合jump时使用。
//返回值用于判定还能否进行进一步的规划。
//如果下一个位置是jump，则时间判定需要满足一整个G
bool Strategy::planDisk(DiskProcessor* diskPcs) {//每次选择一个请求单元最多的开始规划？
    //auto planners = diskPcs->planners;
    
    vector<ReadBlock> firstOptions;
    vector<HeadPlanner*> planners;
    for(int i=0;i<diskPcs->planners.size();i++){
        auto planner = diskPcs->planners[i];
        if(!toBePlanned[diskPcs->disk->diskId][i]){
            planner->freshVRead();
            continue;
        }
        int lastPos = planner->getLastActionNode().endPos;
        int startTokens = std::max<int>(planner->getLastActionNode().endTokens, Watch::getTime()*G);
        auto tempIt = diskPcs->getReqUnitIteratorUnplanedAt(lastPos);
        int dist = getDistance(tempIt.getKey(), lastPos, planner->spaceSize);
        if(dist >= G){//必须JUMP到达
            //准确来说是大于G+当前planner剩余时间步，
            //因此最早在下下个时间步到达。可以尝试抛弃当前行动直接在下个时间步到达。
            
            //比较取消和不取消的区别。当前不取消。
        }else if(dist < G){//通过pass到达，判断是否可以置换。
            int passStartTokens = calNewTokensCost(startTokens, dist, true);
            auto block = diskPcs->getMultiReadBlock(tempIt, passStartTokens);
            firstOptions.push_back(block);
        }
    }

    //选择一个搜索起始位置。
    auto iter = diskPcs->getReqUnitIteratorUnplanedAt(planner->getLastActionNode().endPos);
    if (iter.isEnd()) { return false; }
    //确定一个通用起始tokens
    int startTokens = Watch::getTime()*G;
    for(int i=0;i<planners.size();i++){
        startTokens = std::max<int>(startTokens, planners[i]->getLastActionNode().endTokens);//确定一个起始tokens
    }
    //有两种情况，置换(需用置换条件判断)，跳读(只判断长期收益？)
    //需要避开紧后连读块。用紧后连读块比较。
    int searchNum = 0;
    vector<ReadBlock> readBlockList = {};
    while (++searchNum <= MULTIREAD_SEARCH_NUM) {//对于跳读而言，找到一个最长的读，然后从此处开始判断是否连读。
        //注意这里没有计算前置损耗。前置损耗貌似只能在后面规划时计算。
        //如果通过pass连接的前者和后者满足置换条件，则可以分离考虑，否则放一起考虑？
        //或者为了防止下个时间片pass中间插入其它需要读的块，实现每个时间片开始时planner中只有read和vread？
        auto block = diskPcs->getMultiReadBlock(iter, startTokens);//从时间片开始时计算。
        readBlockList.push_back(block);

        //iter已经指向下一个位置了。//iter.toUnplanedNoLessThan(nextStart);
        if (iter.isEnd()) { break; }// throw std::logic_error("既然已经进入了循环，iter就不会到end"); 
    }


    for(int i=0;i<planners.size();i++){
        
    }


    planner->appendMoveTo(readBlocks.front().first);
    for (int k = 0; k < readBlocks.size(); k++) {
        int start = readBlocks[k].first;
        //至少把第一个规划了。
        for (int j = 0; j < readBlocks[k].second; j++) {
            planner->appendMoveToAllReadAndPlan((start + j) % planner->spaceSize);
            LOG_PLANNER  << " plan for unit "
                << (start + j) % planner->spaceSize;
            LOG_PLANNERN(planner->getDiskId()) << " plan for unit "
                << (start + j) % planner->spaceSize;
        }
        //但是只把当前时间步相关的行动入栈并等待执行。
        if (Watch::toTimeStep(planner->getLastActionNode().endTokens) > Watch::getTime()) {
            //以readBlock为单元加入读。
            break;
        }
    }//也可以直接执行，然后清除未执行完的内容，防止plan的麻烦。
    return true;
}

int DiskProcessor::calNextStart(std::vector<std::pair<int, int>>& readBlocks) {
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

//profit用请求数来计算？还是用分数来计算？先用请求数试试。
//返回{ tolReqNum , tolTokensCost }。计算了移动到第一个起始请求的花费。
//reqIt要么停在下一个未plan的位置，要么为end
ReadBlock DiskProcessor::getMultiReadBlock(DiskProcessor::ReqIterator& reqIt, int startTokens) {
    ReadBlock readBlock;
    ReadBlock tempBlock;
    tempBlock.tokensEnd = startTokens;
    
    int multiReadTokensProfit = 0;
    int multiReadLen = 0;
    int invalidReadLen = 0;
    int tempPos = reqIt.getKey();
    int nextPos = tempPos;
    int gap = 0;
    Object* obj; DiskUnit unitInfo;
    while (gap < MULTIREAD_JUDGE_LENGTH) {//从tempPos开始，是否可以读原本不需要读的块来获取收益。
        if(gap>0){
            multiReadLen = 0;
            invalidReadLen += gap;
            for(int i=0;i<gap;i++){
                multiReadTokensProfit =
                    multiReadTokensProfit
                    - getReadConsumeAfterN(tempBlock.blockLength) + 1;

                tempBlock.tokensEnd = calNewTokensCost(tempBlock.tokensEnd, getReadConsumeAfterN(tempBlock.blockLength), false);
                tempBlock.blockLength++;
            }
        }
        //不论如何，下面内容都会运行。
        {
            unitInfo = disk->getUnitInfo(tempPos);
            obj = sObjectsPtr[unitInfo.objId];
            assert(!obj->isPlaned(unitInfo.untId));
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
        
        if (multiReadTokensProfit >= 0) {//没有连读损耗
            readBlock = tempBlock;
        }//只有在读判断中才有可能大于0。
        
        reqIt.toNextUnplanedReqUnit();
        if(reqIt.isEnd()){break;}
        gap = getDistance(reqIt.getKey(), tempPos, disk->spaceSize) - 1;
        tempPos = reqIt.getKey();
    }
    if(reqIt.isEnd() || gap>=G){
        readBlock.tokensEnd = ((readBlock.tokensEnd-1)/G + 1)*G;//规整时间到时间步起始处。
    }
    return readBlock;
}

//优化方向：修改成未规划的键数量。
bool cmpDiskPcsByReqUnitNum(pair<DiskProcessor*, int>& a, pair<DiskProcessor*, int>& b){
    return a.first->reqSpace.getKeyNum() > b.first->reqSpace.getKeyNum();//谁当前cost最多就先规划谁。
}



#endif