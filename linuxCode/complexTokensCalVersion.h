
        // struct ReadProfitInfo{
        //     int score = 0;
        //     int edge = 0;
        //     int reqNum = 0;
        //     int tokensCost = 0;
        // };
        // //判断是否能够连读。如果能，则连读。
        // //该函数可以找到不同位置开始的较长的连读段。适合jump时使用。
        // bool planMultiRead() {//每次选择一个请求单元最多的开始规划？
        //     int searchNum = 0;
        //     auto iter = getReqUnitIteratorUnplanedAt(planner->getLastActionNode().endPos);
        //     if (iter.isEnd()) { return false; }
        //     std::vector<std::pair<int, int>> readBlocks = {};
        //     if (Watch::getTime()==802) {
        //         int test = 0;
        //     }

        //     int startReqUnit = iter.getKey();
        //     if (getDistance(startReqUnit, planner->getLastActionNode().endPos, this->disk->spaceSize) >= G) {
        //         int reqUnit = startReqUnit;//第一个请求位置
        //         double maxProfit = 0;
        //         double tempProfit = 0;
        //         int startTokens = calNewTokensCost(planner->getLastActionNode().endTokens, G, false); 
        //         while (true) {//对于跳读而言，找到一个最长的读，然后从此处开始判断是否连读。
        //             std::vector<std::pair<int, int>> tempReadBlocks = {};
        //             if (reqUnit == 747) {
        //                 int test = 0;
        //             }
        //             auto info = getReadProfitUntilJump(reqUnit, startTokens, tempReadBlocks);//一直使用这个startTokens
        //             //tempProfit = info.score*1.0 / (info.tokensCost-startTokens);
        //             tempProfit = info.reqNum * 1.0 / (info.tokensCost - startTokens);
        //             int nextStart = (tempReadBlocks.back().second + tempReadBlocks.back().first) % this->disk->spaceSize;
        //             if(maxProfit < tempProfit){
        //                 readBlocks = std::move(tempReadBlocks);
        //                 maxProfit = tempProfit;
        //             }
        //             if (++searchNum >= MULTIREAD_SEARCH_NUM) {
        //                 break;
        //             }
        //             iter = getReqUnitIteratorUnplanedAt(nextStart);
                    
        //             if (iter.isEnd()) { throw std::logic_error("既然已经进入了循环，iter就不会到end"); }
        //             reqUnit = iter.getKey();//获取下一个未规划的单元。
        //             if (startReqUnit == reqUnit) {//如果转了一圈
        //                 break;
        //             }
        //         }
        //     }
        //     else {//判断是继续读还是直接跳。
        //         int startTokens = planner->getLastActionNode().endPos;
        //         int passTokens = getDistance(startReqUnit, startTokens, this->disk->spaceSize);
        //         int startTokensOfPass = calNewTokensCost(startTokens, passTokens, true);
        //         auto passProfitInfo = getReadProfitUntilJump(startReqUnit, startTokensOfPass, readBlocks);

        //         int nextReqUnit = readBlocks.back().first + readBlocks.back().second;
        //         auto iter = getReqUnitIteratorUnplanedAt(nextReqUnit);
        //         int startReqUnit = iter.getKey();
        //         //检查跳是否能取得更高收益。
        //         int reqUnit = startReqUnit;
        //         int maxProfit = 0;
        //         int startTokensOfJump = calNewTokensCost(startTokens, G, true);
        //         while (true) {//对于跳读而言，找到一个最长的读，然后从此处开始判断是否连读。
        //             std::vector<std::pair<int, int>> tempReadBlocks = {};
        //             auto info = getReadProfitUntilJump(reqUnit, startTokensOfJump, tempReadBlocks);
        //             //int passLoss = passProfitInfo.edge * passTokens + info.edge * (passTokens + passProfitInfo.tokensCost - startTokens);
        //             //int jumpLoss = G * info.edge + (G + info.tokensCost - startTokens) * passProfitInfo.edge;
        //             int passLoss = passProfitInfo.reqNum * passTokens + info.reqNum * (passTokens + passProfitInfo.tokensCost - startTokens);
        //             int jumpLoss = G * info.reqNum + (G + info.tokensCost - startTokens) * passProfitInfo.reqNum;
        //             int nextStart = (tempReadBlocks.back().second + tempReadBlocks.back().first) % this->disk->spaceSize;
        //             if (passLoss - jumpLoss > maxProfit) {
        //                 maxProfit = passLoss - jumpLoss;
        //                 readBlocks = std::move(tempReadBlocks);
        //             }
        //             if (++searchNum >= MULTIREAD_SEARCH_NUM) {
        //                 break;
        //             }
        //             iter = getReqUnitIteratorUnplanedAt(nextStart);
        //             if (iter.isEnd()) { throw std::logic_error("既然已经进入了循环，iter就不会到end"); }
        //             reqUnit = iter.getKey();//获取下一个未规划的单元。
        //             if (startReqUnit == reqUnit) {//如果转了一圈
        //                 break;
        //             }
        //         }
        //     }
            
        //     planner->appendMoveTo(readBlocks.front().first);
        //     LOG_PLANNERN(planner->getDiskId()) << "\nbefore add, planner:" << planner;
        //     LOG_PLANNER << "planner " << disk->diskId << " " << planner->getDiskId() <<" planning ";
        //     for (int k = 0; k < readBlocks.size(); k++) {
        //         int start = readBlocks[k].first;
        //         //但是只把当前时间步相关的行动入栈并等待执行。
        //         if (Watch::toTimeStep(planner->getLastActionNode().endTokens) <= Watch::getTime()) {
        //             //以readBlock为单元加入读。
        //             for (int j = 0; j < readBlocks[k].second; j++) {
        //                 planner->appendMoveToAllReadAndPlan((start + j) % disk->spaceSize);
        //                 LOG_PLANNER  << " plan for unit "
        //                     << (start + j) % disk->spaceSize;
        //                 LOG_PLANNERN(planner->getDiskId()) << " plan for unit "
        //                     << (start + j) % disk->spaceSize;
        //             }
        //         }
        //         else {
        //             for (int j = 0; j < readBlocks[k].second; j++) {
        //                 auto info = disk->getUnitInfo((start + j) % disk->spaceSize);
        //                 auto obj = sObjectsPtr[info.objId];
        //                 obj->plan(info.untId, disk->diskId);//为当前时间步规划。防止其它磁盘也用到该磁盘现在规划的单元。
        //                 LOG_OBJECT << "obj " << obj->objId << " plan for unit " 
        //                     << info.untId << " in pos " << (start + j) % disk->spaceSize << " on disk " << this->disk->diskId;
        //                 LOG_PLANNER << "obj " << obj->objId << " plan for unit "
        //                     << info.untId << " in pos " << (start + j) % disk->spaceSize << " on disk " << this->disk->diskId;
        //             }
        //         }
        //     }//也可以直接执行，然后清除未执行完的内容，防止plan的麻烦。
        //     LOG_PLANNERN(planner->getDiskId()) << "after add, planner:" << planner;
        //     return true;
        // }
        // /// @brief 获取从某一位置直到下一个JUMP位置的读收益
        // /// @param start 起始位置
        // /// @param readBlocks 用于接收该区间内的连读块
        // /// @return 读收益
        // ReadProfitInfo getReadProfitUntilJump(int start, int startTokens,
        //         std::vector<std::pair<int,int>>& readBlocks) {
        //     std::set<Object*> relatedObjects = {};
        //     int tolTokensCost = startTokens;
        //     auto iter = getReqUnitIteratorUnplanedAt(start);
        //     if (iter.isEnd()) { return {0, 1}; }
        //     int cost = getDistance(iter.getKey(), start, this->disk->spaceSize); 
        //     assert(cost == 0);
        //     while (!iter.isEnd() && 
        //             getDistance(iter.getKey(), start, this->disk->spaceSize) < G &&
        //             readBlocks.size() <= 20) {
        //         tolTokensCost = calNewTokensCost(tolTokensCost, 
        //             getDistance(iter.getKey(), start, this->disk->spaceSize),
        //             true);//加上pass的距离。
        //         if (tolTokensCost == 281050 && start == 747) {
        //             int test = 0;
        //         }
        //         int length = getMultiReadBlock(iter.getKey(), tolTokensCost, &tolTokensCost, &relatedObjects);
        //         readBlocks.push_back({ start, length });

        //         start = start + length;
        //         auto iter = getReqUnitIteratorUnplanedAt(start);//会找到下一个或者和start相等的。
        //     }
        //     LOG_PLANNER << "read blocks size:" << readBlocks.size();

        //     int score = 0; int edge = 0; int reqNum = 0;
        //     for(auto it = relatedObjects.begin();it!=relatedObjects.end();it++){
        //         (*it)->calScoreAndClearPlanBuffer(&score, &edge, &reqNum);
        //     }
        //     return { score , edge, reqNum, tolTokensCost };
        // }
        // /// @brief 获取一个待规划的连读块。该函数会判断是否可以通过读非请求块减少连读开销。
        // /// 同时会设置对象的临时planBuffer标识连读块中读行动的结束时间步。
        // /// @param reqUnit 请求单元起始位置，需要保证是未被规划的请求单元
        // /// @param startTokens 当前的时间累积令牌位置，通过Watch::toTimeStep可以转化为时间步
        // /// @param getTokensCost 获取结束时的令牌花费
        // /// @param relatedObjects 用于接收相关对象
        // /// @return 连读块的长度
        // int getMultiReadBlock(int reqUnit, int startTokens, int* getTokensCost, 
        //     std::set<Object*>* relatedObjects) {
        //     int tempPos = reqUnit;

        //     int readLength = 0;
        //     int tolMultiReadLen = 0;
        //     int tolTokensCost = startTokens;
        //     int maxMultiReadBlockLength = 0;
        //     int maxMultiReadValidLength = 0;
        //     int maxMultiReadTokensCost = 0;
            
        //     int multiReadTokensProfit = 0;
        //     int multiReadLen = 0;
        //     int invalidReadLen = 0;
        //     bool lastJudgeRead = false;
        //     DiskUnit unitInfo = this->disk->getUnitInfo(tempPos);
        //     Object* obj = sObjectsPtr[unitInfo.objId];
        //     while (invalidReadLen < 8) {//从tempPos开始，是否可以读原本不需要读的块来获取收益。
        //         readLength++;
        //         if (obj != &deletedObject && obj != nullptr &&//该处有对象且未被删除
        //                 obj->unitReqNum[unitInfo.untId] > 0 && //判断是否有请求
        //                 (!obj->isPlaned(unitInfo.untId))) {
        //             relatedObjects->insert(obj);

        //             lastJudgeRead = true;
        //             tolTokensCost = calNewTokensCost(tolTokensCost, 
        //                                         getReadConsumeAfterN(readLength-1), false);
        //             obj->setUnitCompleteStepInPlanBuffer(unitInfo.untId, Watch::toTimeStep(tolTokensCost));
        //             tolMultiReadLen++;
        //             multiReadLen++;
        //             invalidReadLen = 0;
                    
        //             multiReadTokensProfit = std::min(0, 
        //                 multiReadTokensProfit
        //                 + getReadConsumeAfterN(multiReadLen)
        //                 - getReadConsumeAfterN(readLength));
        //         }
        //         else {
        //             lastJudgeRead = false;
        //             tolTokensCost += getReadConsumeAfterN(multiReadLen-1);
        //             invalidReadLen++;
        //             multiReadLen = 0;
                    
        //             multiReadTokensProfit =
        //                 multiReadTokensProfit
        //                 - getReadConsumeAfterN(readLength) + 1;
        //         }
        //         if (multiReadTokensProfit >= 0) {//没有连读损耗
                    
        //             maxMultiReadBlockLength = readLength;//那么就连读到该位置。
        //             maxMultiReadValidLength = tolMultiReadLen;
        //             maxMultiReadTokensCost = tolTokensCost;
        //         }
        //         tempPos = (tempPos + 1) % this->disk->spaceSize; //查看下一个位置是否未被规划。
        //         unitInfo = this->disk->getUnitInfo(tempPos);
        //         obj = sObjectsPtr[unitInfo.objId];
        //     }
        //     *getTokensCost = maxMultiReadTokensCost;
        //     return maxMultiReadBlockLength;
        // }
        