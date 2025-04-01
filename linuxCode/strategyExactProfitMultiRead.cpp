#include "diskManager.h"
#include "common.h"


//�ж��Ƿ��ܹ�����������ܣ���������
//�ú��������ҵ���ͬλ�ÿ�ʼ�Ľϳ��������Ρ��ʺ�jumpʱʹ�á�
bool DiskProcessor::planMultiReadWithExactInfo() {//ÿ��ѡ��һ������Ԫ���Ŀ�ʼ�滮��
    int searchNum = 0;
    auto iter = getReqUnitIteratorUnplanedAt(planner->getLastActionNode().endPos);
    if (iter.isEnd()) { return false; }
    std::vector<std::pair<int, int>> readBlocks = {};
    if (Watch::getTime() == 75) {
        int test = 0;
    }
    int startReqUnit = iter.getKey();
    if (getDistance(startReqUnit, planner->getLastActionNode().endPos, this->disk->spaceSize) >= G) {
        int reqUnit = startReqUnit;//��һ������λ��
        double maxProfit = 0;
        double tempProfit = 0;
        int startTokens = calNewTokensCost(planner->getLastActionNode().endTokens, G, false);
        while (true) {//�����������ԣ��ҵ�һ����Ķ���Ȼ��Ӵ˴���ʼ�ж��Ƿ�������
            std::vector<std::pair<int, int>> tempReadBlocks = {};
            auto info = getExactReadProfitUntilJump(reqUnit, startTokens, tempReadBlocks);//һֱʹ�����startTokens
            //tempProfit = info.score*1.0 / (info.tokensCost-startTokens);
            tempProfit = info.score * 1.0 / (info.tokensCost - startTokens);
            int nextStart = (tempReadBlocks.back().second + tempReadBlocks.back().first) % this->disk->spaceSize;
            if (maxProfit < tempProfit) {
                readBlocks = std::move(tempReadBlocks);
                maxProfit = tempProfit;
            }
            if (++searchNum >= MULTIREAD_SEARCH_NUM) {
                break;
            }
            iter = getReqUnitIteratorUnplanedAt(nextStart);

            if (iter.isEnd()) { throw std::logic_error("��Ȼ�Ѿ�������ѭ����iter�Ͳ��ᵽend"); }
            reqUnit = iter.getKey();//��ȡ��һ��δ�滮�ĵ�Ԫ��
            if (startReqUnit == reqUnit) {//���ת��һȦ
                break;
            }
        }
    }
    else {//�ж��Ǽ���������ֱ������
        int startTokens = planner->getLastActionNode().endPos;
        int passTokens = getDistance(startReqUnit, startTokens, this->disk->spaceSize);
        int startTokensOfPass = calNewTokensCost(startTokens, passTokens, true);
        auto passProfitInfo = getExactReadProfitUntilJump(startReqUnit, startTokensOfPass, readBlocks);

        int nextReqUnit = readBlocks.back().first + readBlocks.back().second;
        auto iter = getReqUnitIteratorUnplanedAt(nextReqUnit);
        int startReqUnit = iter.getKey();
        //������Ƿ���ȡ�ø������档
        int reqUnit = startReqUnit;
        int maxProfit = 0;
        int startTokensOfJump = calNewTokensCost(startTokens, G, true);
        while (true) {//�����������ԣ��ҵ�һ����Ķ���Ȼ��Ӵ˴���ʼ�ж��Ƿ�������
            std::vector<std::pair<int, int>> tempReadBlocks = {};
            auto info = getExactReadProfitUntilJump(reqUnit, startTokensOfJump, tempReadBlocks);
            //int passLoss = passProfitInfo.edge * passTokens + info.edge * (passTokens + passProfitInfo.tokensCost - startTokens);
            //int jumpLoss = G * info.edge + (G + info.tokensCost - startTokens) * passProfitInfo.edge;
            int passLoss = passProfitInfo.edge * passTokens + info.edge * (G + passProfitInfo.tokensCost - startTokens);
            int jumpLoss = info.edge * G + passProfitInfo.edge * (G + info.tokensCost - startTokens);
            int nextStart = (tempReadBlocks.back().second + tempReadBlocks.back().first) % this->disk->spaceSize;
            if (passLoss - jumpLoss > maxProfit) {
                maxProfit = passLoss - jumpLoss;
                readBlocks = std::move(tempReadBlocks);
            }
            if (++searchNum >= MULTIREAD_SEARCH_NUM) {
                break;
            }
            iter = getReqUnitIteratorUnplanedAt(nextStart);
            if (iter.isEnd()) { throw std::logic_error("��Ȼ�Ѿ�������ѭ����iter�Ͳ��ᵽend"); }
            reqUnit = iter.getKey();//��ȡ��һ��δ�滮�ĵ�Ԫ��
            if (startReqUnit == reqUnit) {//���ת��һȦ
                break;
            }
        }
    }

    planner->appendMoveTo(readBlocks.front().first);
    LOG_PLANNERN(planner->getDiskId()) << "\nbefore add, planner:" << planner;
    LOG_PLANNER << "planner " << disk->diskId << " " << planner->getDiskId() << " planning ";
    for (int k = 0; k < readBlocks.size(); k++) {
        int start = readBlocks[k].first;
        //����ֻ�ѵ�ǰʱ�䲽��ص��ж���ջ���ȴ�ִ�С�
        if (Watch::toTimeStep(planner->getLastActionNode().endTokens) <= Watch::getTime()) {
            //��readBlockΪ��Ԫ�������
            for (int j = 0; j < readBlocks[k].second; j++) {
                planner->appendMoveToAllReadAndPlan((start + j) % disk->spaceSize);
                LOG_PLANNER << " plan for unit "
                    << (start + j) % disk->spaceSize;
                LOG_PLANNERN(planner->getDiskId()) << " plan for unit "
                    << (start + j) % disk->spaceSize;
            }
        }
        else {
            for (int j = 0; j < readBlocks[k].second; j++) {
                auto info = disk->getUnitInfo((start + j) % disk->spaceSize);
                auto obj = sObjectsPtr[info.objId];
                obj->plan(info.untId, disk->diskId);//Ϊ��ǰʱ�䲽�滮����ֹ��������Ҳ�õ��ô������ڹ滮�ĵ�Ԫ��
                LOG_OBJECT << "obj " << obj->objId << " plan for unit "
                    << info.untId << " in pos " << (start + j) % disk->spaceSize << " on disk " << this->disk->diskId;
                LOG_PLANNER << "obj " << obj->objId << " plan for unit "
                    << info.untId << " in pos " << (start + j) % disk->spaceSize << " on disk " << this->disk->diskId;
            }
        }
    }//Ҳ����ֱ��ִ�У�Ȼ�����δִ��������ݣ���ֹplan���鷳��
    LOG_PLANNERN(planner->getDiskId()) << "after add, planner:" << planner;
    return true;
}
/// @brief ��ȡ��ĳһλ��ֱ����һ��JUMPλ�õĶ�����
/// @param start ��ʼλ��
/// @param readBlocks ���ڽ��ո������ڵ�������
/// @return ������
ReadProfitInfo DiskProcessor::getExactReadProfitUntilJump(int start, int startTokens, std::vector<std::pair<int, int>>& readBlocks) {
    std::set<Object*> relatedObjects = {};
    int tolTokensCost = startTokens;  int tolNum = 0;
    auto iter = getReqUnitIteratorUnplanedAt(start);
    if (iter.isEnd()) { return { 0, 1 }; }
    int cost = getDistance(iter.getKey(), start, disk->spaceSize);
    assert(cost == 0);
    LOG_PLANNER << "\nstart at:" << iter.getKey();
    while (!iter.isEnd() &&
        getDistance(iter.getKey(), start, disk->spaceSize) < G &&
        readBlocks.size() <= 5) {
        tolTokensCost = calNewTokensCost(tolTokensCost,
            getDistance(iter.getKey(), start, disk->spaceSize),
            true);//����pass�ľ��롣
        start = iter.getKey(); LOG_PLANNER << "move to:" << iter.getKey();
        //tolTokensCost += getDistance(iter.getKey(), start, this->disk->spaceSize);
        int getReqNum = 0;
        int length = getMultiReadBlockAndRelatedObjects(iter.getKey(), tolTokensCost, &tolTokensCost, &relatedObjects, &getReqNum);
        tolNum += getReqNum;
        readBlocks.push_back({ start, length });

        start = (start + length)%disk->spaceSize;
        LOG_PLANNER << "block end:" << start;
        iter.toUnplanedNoLessThan(start);
        //LOG_PLANNER << "nextUnplaned:" << iter.getKey();
        //û��ǰ����һ��δ�滮��λ�ã�����
        //���iter�����������鷳��������
    }
    LOG_PLANNER << "read blocks size:" << readBlocks.size() <<"\n\n";

    int score = 0; int edge = 0; int reqNum = 0;
    for (auto it = relatedObjects.begin(); it != relatedObjects.end(); it++) {
        if (Watch::getTime() == 75) {
            int test = 0;
        }
        (*it)->calScoreAndClearPlanBuffer(&score, &edge, &reqNum);
    }
    assert(tolNum == reqNum);
    
    ReadProfitInfo newInfo;
    newInfo.reqNum = reqNum; newInfo.score = score; newInfo.edge = edge; newInfo.tokensCost = tolTokensCost;
    return newInfo;
}
/// @brief ��ȡһ�����滮�������顣�ú������ж��Ƿ����ͨ������������������������
/// ͬʱ�����ö������ʱplanBuffer��ʶ�������ж��ж��Ľ���ʱ�䲽��
/// @param reqUnit ����Ԫ��ʼλ�ã���Ҫ��֤��δ���滮������Ԫ
/// @param startTokens ��ǰ��ʱ���ۻ�����λ�ã�ͨ��Watch::toTimeStep����ת��Ϊʱ�䲽
/// @param getTokensCost ��ȡ����ʱ�����ƻ���
/// @param relatedObjects ���ڽ�����ض���
/// @return ������ĳ���
int DiskProcessor::getMultiReadBlockAndRelatedObjects(int reqUnit, int startTokens, int* getTokensCost, std::set<Object*>* relatedObjects, int* getReqNum) {
    int tempPos = reqUnit;

    int reqNum = 0;

    int readLength = 0;
    int tolMultiReadLen = 0;
    int tolTokensCost = startTokens;
    int maxMultiReadBlockLength = 0;
    int maxMultiReadValidLength = 0;
    int maxMultiReadTokensCost = startTokens;

    int multiReadTokensProfit = 0;
    int multiReadLen = 0;
    int invalidReadLen = 0;
    bool lastJudgeRead = false;
    std::vector<DiskUnit> unitInfos = {};
    std::vector<int> unitCompTokens = {};
    DiskUnit unitInfo = this->disk->getUnitInfo(tempPos);
    Object* obj = sObjectsPtr[unitInfo.objId];
    while (invalidReadLen < 8) {//��tempPos��ʼ���Ƿ���Զ�ԭ������Ҫ���Ŀ�����ȡ���档
        if (obj != deletedObject && obj != nullptr &&//�ô��ж�����δ��ɾ��
            obj->unitReqNum[unitInfo.untId] > 0 && //�ж��Ƿ�������
            (!obj->isPlaned(unitInfo.untId))) {
            tolTokensCost = calNewTokensCost(tolTokensCost,
                getReadConsumeAfterN(readLength), false);
            //tolTokensCost += getReadConsumeAfterN(readLength);
            multiReadTokensProfit = std::min(0,
                multiReadTokensProfit
                + getReadConsumeAfterN(multiReadLen)
                - getReadConsumeAfterN(readLength));
            readLength++;
            tolMultiReadLen++;
            multiReadLen++;
            invalidReadLen = 0;
            lastJudgeRead = true;
            reqNum += obj->unitReqNum[unitInfo.untId];
            unitInfos.push_back(unitInfo);
            unitCompTokens.push_back(tolTokensCost);
            
        }
        else {
            tolTokensCost = calNewTokensCost(tolTokensCost,
                getReadConsumeAfterN(readLength), false);
            //tolTokensCost += getReadConsumeAfterN(readLength);
            multiReadTokensProfit =
                multiReadTokensProfit
                - getReadConsumeAfterN(readLength) + 1;
            readLength++;
            lastJudgeRead = false;
            invalidReadLen++;
            multiReadLen = 0;
        }
        if (multiReadTokensProfit >= 0) {//û���������
            for (int i = maxMultiReadValidLength; i < tolMultiReadLen; i++) {
                Object* obj = sObjectsPtr[unitInfos[i].objId];
                obj->setUnitCompleteStepInPlanBuffer(unitInfos[i].untId, Watch::toTimeStep(unitCompTokens[i]));
                relatedObjects->insert(obj);
            }
            maxMultiReadBlockLength = readLength;//��ô����������λ�á�
            maxMultiReadValidLength = tolMultiReadLen;
            maxMultiReadTokensCost = tolTokensCost;
            *getReqNum = reqNum;
        }
        tempPos = (tempPos + 1) % this->disk->spaceSize; //�鿴��һ��λ���Ƿ�δ���滮��
        unitInfo = disk->getUnitInfo(tempPos);
        obj = sObjectsPtr[unitInfo.objId];
    }
    *getTokensCost = maxMultiReadTokensCost;
    return maxMultiReadBlockLength;
}

void DiskManager::exactMultiReadStrategy() {
    //����ÿ����Ԫ�ڶ�Ӧ�����������ʱ�䡣�����жϵ�Ԫ�Ƿ��֮ǰ�ĵ�Ԫ��ͬһ�����ϣ�
    LOG_DISK << "timestamp " << Watch::getTime() << " plan disk:";
    std::vector<DiskProcessor*> diskPcsVec;
    DiskProcessor* diskPcs;
    //�����һ���ж������Ұ�����ָ��ת����ǰheadλ�á�
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
        //�ҵ����д��̵ģ�����ڴ�ͷ����һ��δ�滮plan,�Լ���Ӧ��object
        std::sort(diskPcsVec.begin(), diskPcsVec.end(), cmpDiskPcsByReqUnitNum);
        for (int i = 0; i < diskPcsVec.size(); i++) {
            auto diskPcs = diskPcsVec[i];
            if (Watch::toTimeStep(diskPcs->planner->getLastActionNode().endTokens) > Watch::getTime() + PLAN_STEP - 1) {
                diskPcsVec.erase(diskPcsVec.begin() + i);
                i--;
                continue;
            }
            if (!diskPcs->planMultiReadWithExactInfo()) {
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
