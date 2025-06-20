﻿#ifndef HEADPLANNER_H
#define HEADPLANNER_H

#include "disk.h"
#include "common.h"

class ActionNode;
class HeadPlanner;
LogStream& operator<<(LogStream& s, const ActionNode& actionNode);
LogStream& operator<<(LogStream& s, HeadPlanner& headPlanner);


struct ActionNode {
    HeadOperator action;
    int endPos;
    int endTokens;
};

class DiskProcessor;
class HeadPlanner {//全局plan，其中的tokens为全局tokens。
private:
    friend LogStream& operator<<(LogStream& s, HeadPlanner& headPlanner);
    //属于哪个头的规划器
    DiskHead* head;
    //分支树
    std::map<int, std::pair<std::list<ActionNode>::iterator, HeadPlanner*>> readBranches = {};
    //分支行动。分别为：增加读的位置/创建分支的位置/分支策略。
    //三元组<HeadOperator, int, int>:磁头操作/操作结束位置/操作结束时间。
    std::list<ActionNode> actionNodes = {};//第零个是起始node。
public:
    const int spaceSize;
    const int diskId;
    DiskProcessor* diskPcs;

    // HeadPlanner(int readConsume, int headpos, int spacesize):
    //     nextReadConsume(readConsume), orgHeadPos(headpos), spaceSize(spacesize){}
public:
    HeadPlanner(DiskProcessor* diskPcs, DiskHead* diskHead) :
        spaceSize(diskHead->spaceSize),
        diskId(diskHead->diskId),
        diskPcs(diskPcs),
        head(diskHead)
    {
        if (head->tokensOffset != 0) {
            throw std::logic_error("don't initialize headPlanner by head with action incompleted.");
        }
        actionNodes.push_back({ {START, getAheadReadTimes(head->readConsume)}, head->headPos, G - head->presentTokens });
        assert(G - head->presentTokens == 0);
    }
    //用一个actionNode来创建分支。
    HeadPlanner(DiskProcessor* diskPcs, DiskHead* diskHead, const ActionNode& actionNode) :
        spaceSize(diskHead->spaceSize),
        diskId(diskHead->diskId),
        diskPcs(diskPcs),
        head(diskHead)
    {
        int aheadRead = 0;
        if (actionNode.action.action == READ || actionNode.action.action == VREAD) {
            aheadRead = actionNode.action.aheadRead + 1;
        }
        else if (actionNode.action.action == START) {
            aheadRead = actionNode.action.aheadRead;
        }
        actionNodes.push_back({ {START, aheadRead},
                                actionNode.endPos,
                                actionNode.endTokens });
        if (head->tokensOffset != 0) {
            throw std::logic_error("don't initialize headPlanner by head with action incompleted.");
        }
        //actionNodes.push_back({{START, 0}, disk->head.headPos, G - disk->head.presentTokens});
    }
    //不要用还未完成行动的head来初始化HeadActionsInfo！
    // 将操作序列附加到现有操作操作序列中，并更新所有操作的累积花费和读操作状态。
    // @param actions 要附加的操作序列。
    const ActionNode& getLastActionNode() {
        return actionNodes.back();
    }
    const std::list<ActionNode>& getActionNodes() {
        return this->actionNodes;
    }
    const int& getDiskId() { return this->diskId; }
    void appendActions(const std::vector<HeadOperator>& actions) {
        for (int i = 0; i < actions.size(); i++) {
            auto operation = actions[i];
            appendAction(operation);
        }
    }
    void appendActions(const std::list<HeadOperator>& actions) {
        for (auto it = actions.begin(); it != actions.end(); it++) {
            auto operation = *it;
            appendAction(operation);
        }
    }
    
    /// @brief 规划一个磁头行动，会自动计算连读信息、行动结束时的tokens花费和磁头位置
    /// @param operation 提供的磁头行动
    /// @param ignoreAheadReadNum 如果提供的磁头行动是READ，是否不检查READ的连读次数正确性
    void appendAction(const HeadOperator& operation, bool ignoreAheadReadNum = false) {
        LOG_PLANNERN(this->diskId) << "append action " << operation;
        if (this->readBranches.size()) {
            std::logic_error("should not append when has branch");
        }
        auto action = operation.action;
        if (action == READ||action == VREAD) {//对于READ操作
            int aheadRead = getAheadReadNum(getLastActionNode());

            if (operation.aheadRead != -1 && (!ignoreAheadReadNum)) {
                assert(operation.aheadRead == aheadRead);
            }
            this->pushBackActionNode({ action, aheadRead }, 1, true,
                getReadConsumeAfterN(aheadRead), false);
        }
        else if (action == PASS) {
            //PASS不需要细分操作数，因为不需要判断某个单元是否有某个对象在赶往，
            // 只需要知道某个单元什么时候会被读取即可。
            this->pushBackActionNode(operation, operation.passTimes, true,
                operation.passTimes, true);
        }
        else if (action == JUMP) {
            if (operation.jumpTo < 0 || operation.jumpTo >= spaceSize) {
                throw std::out_of_range("action JUMP jumps out of range");
            }
            this->pushBackActionNode(operation, operation.jumpTo, false, G, false);
        }
        else {
            throw std::logic_error("action wrong!");
        }

    }
    /// @brief 放入一个行动节点
    /// @param operation 行动类型及对应行动参数
    /// @param posValue 位置参数
    /// @param isOffset 是否是偏移位置。对于JUMP而言，需要设置为false
    /// @param costValue 花费的Tokens
    /// @param divisible 行动相对于花费的Tokens是否可分。对于PASS而言，可分。
    void pushBackActionNode(const HeadOperator& operation, int posValue, bool isOffset, int costValue, bool divisible) {
        auto lastNode = getLastActionNode();
        int newCost = calNewTokensCost(lastNode.endTokens, costValue, divisible);
        int newPos = calNewHeadPos(lastNode.endPos, posValue, isOffset, this->spaceSize);
        actionNodes.push_back({ operation, newPos, newCost });
        LOG_PLANNERN(diskId) << "push back node " << actionNodes.back();
    }
    /// @brief 获取前面连读的次数。对于READ而言，为上一次的前置连读+1，对于头节点而言，就是头节点的前置连读。
    /// @param lastNode 上一个规划节点
    /// @return 前置连读次数
    int getAheadReadNum(const ActionNode& lastNode) {
        int aheadRead = 0;
        if (lastNode.action.action == READ || lastNode.action.action == VREAD) {
            aheadRead = lastNode.action.aheadRead + 1;
        }
        else if (lastNode.action.action == START) {
            aheadRead = lastNode.action.aheadRead;
        }
        return aheadRead;
    }

    /// @brief 规划移动到某个位置并进行有效读
    /// @param unitPos 读的位置
    void appendMoveToUnplannedReadAndPlan(int unitPos);
    /// @brief 规划移动到某个位置并进行有效或无效读，读的类型由函数根据object判断。
    /// @param unitPos 读的位置
    void appendMoveToAllReadAndPlan(int unitPos);
    /// @brief 规划移动到某个位置。如果尝试进行读来移动，需要提供磁头之后的行为信息。
    /// @param unitPos 移动到的位置
    /// @param tryToRead 是否尝试进行无效读来移动磁头（优化方向：可以全设置为有效读，重点在于得同时设置对象的planed属性，在特定情况下可以优化开销）
    /// @param actionIt 磁头之后的行为起始处迭代器
    /// @param endIt 磁头之后的行为结束处迭代器
    void appendMoveTo(int unitPos, bool tryToRead = false, 
        std::list<ActionNode>::iterator* actionIt=nullptr, 
        std::list<ActionNode>::iterator* endIt = nullptr) {
        LOG_PLANNERN(this->diskId) << "append move to " << unitPos;
        if (this->readBranches.size()) {
            std::logic_error("should not append when has branch");
        }
        int distance = getDistance(unitPos, this->getLastActionNode().endPos, spaceSize);//choseRep已经算过了一遍，这里再算一遍？！
        assert(distance >= 0);
        if (distance) {
            int leftTokens = G - this->getLastActionNode().endTokens % G;
            if (tryToRead) {
                auto headOperation = this->getLastActionNode().action;
                int aheadRead = (headOperation.action == READ|| headOperation.action == VREAD) ? headOperation.aheadRead + 1 : 
                                ((headOperation.action == START) ? headOperation.aheadRead : 0);
                int loss = 0;
                for (int i = 0; i < distance; i++) {
                    loss += getNextReadConsume(aheadRead++) -1;
                }
                if (actionIt == nullptr || endIt == nullptr) {
                    throw std::logic_error("should give all params");
                }
                int multiReadLen = 0; auto it = *actionIt;//就是需要复制。
                for (int i=0; it != *endIt && i<8; i++) {
                    auto action = (*it).action.action;
                    if (action == READ || action == VREAD) {
                        multiReadLen++;
                        if (action == READ) {
                            loss -= getNextReadConsume(multiReadLen) - getNextReadConsume(aheadRead + multiReadLen);
                        }
                    }
                    it++;
                }
                if (loss < 0) {
                    for (int i = 0; i < distance; i++) {
                        this->appendAction(HeadOperator{ VREAD, -1 }, true);
                    }
                }
                else {
                    tryToRead = false;
                }
            }
            if (!tryToRead) {
                if (distance <= leftTokens) {
                    //如果可以直接移动到目标位置。
                    this->appendAction(HeadOperator{ PASS, distance });
                    //也可以试试能不能连读过去（节省时间），这取决于下面有多少单元，能否节省总开销。
                }
                else if (leftTokens == G) {
                    //当前还有G个Token，即上一时间步的操作没有延续到当前时间步。可以直接跳过去。
                    this->appendAction(HeadOperator{ JUMP, unitPos });
                }
                else if (leftTokens + G > distance) {//如果在下个时间片能够到达位置
                    //如果当前时间步的令牌数不够跳，且这个时间步+下个时间步能够移动到目标位置，
                    //那么直接向目标位置移动
                    this->appendAction(HeadOperator{ PASS, leftTokens });
                    this->appendAction(HeadOperator{ PASS, distance - leftTokens });
                }
                else {
                    //最早也得下两个回合才能到达读位置。直接放入跳。
                    this->appendAction(HeadOperator{ JUMP, unitPos });
                }
            }
        }
        else {
            return;//就在当前位置。
        }
    }
    //如果存在VRead被请求，则将其转换为Read并进行plan
    void freshVRead();
    void cancelAllAction();
    // int getTokenCost(){return tokenCost;}
    // int getTimeCost(){return timeCost;}
    // int getNextReadConsume(){return nextReadConsume;}
    // std::vector<std::pair<HeadAction, int>> getActions(){return actions;}

    //规划性质：插入读操作，策略为选择最近插入位置。
    void insertUnplannedReadAsBranch(int unitPos, int* getReadOverTokens, int* getScoreLoss);
    //用分支修改主干,会更新planed值。
    void mergeUnplannedReadBranch(int unitPos);
    //删除分支
    void dropReadBranch(int unitPos);
    
    
    /// @brief 取消特定的有效读操作，并将该读替换为移动/跳/无效读。
    /// 注意！该操作不应放在当前回合操作结束后使用！！否则会导致tokens计算异常！！（当前时间片行动已结束
    /// ，但使用的最低时间还是当前时间片的时间，导致不能正常跳到下一时间片开始处的tokens）
    /// @param unitPos 要删除的读操作的读位置
    void cancelRead(int unitPos) {
        LOG_PLANNERN(diskId) << "cancel read " << unitPos;
        ActionNode node;
        auto it = std::next(this->actionNodes.begin());
        int unitEnd = (unitPos + 1) % spaceSize;
        while(it!=actionNodes.end()) {
            if ((*it).endPos == unitEnd && (*it).action.action == READ) {
                break;
            }
            it++;
        }
        if (it == actionNodes.end()) {
            throw std::logic_error("error! can't find read!!");
        }
        //it为需要删除的节点。
        std::list<ActionNode> tempNodes = {};
        (*it).action.action = VREAD;//暂时只这样处理。因为这里存在bug：
        //由于我判断了是将READ替换为VREAD还是PASS，但是这也会影响到之后的判断，
        //譬如之后也要将一部分VREAD替换为PASS才有效益。
        //此外，变化的READ完成时间也会影响req。因此如果采用上述方式也需要重新plan

        // it = actionNodes.erase(it);
        // tempNodes.splice(tempNodes.begin(), actionNodes, it, actionNodes.end());
        
        // //删除read之前的移动操作
        // it = actionNodes.end(); it--;
        // while (((*it).action.action != READ && (*it).action.action != VREAD) && (*it).action.action != START) {
        //     it = actionNodes.erase(it);
        //     it--;
        // }
        // //更新后面的节点
        // it = tempNodes.begin();
        // auto endIt = tempNodes.end();
        // while (it != endIt) {
        //     if ((*it).action.action == READ || (*it).action.action == VREAD) {
        //         appendMoveTo(((*it).endPos - 1+spaceSize)%spaceSize, true, &it, &endIt);
        //         appendAction((*it).action, true);
        //     }
        //     it++;
        // }
    }
    

    //更新内部状态
    /// @brief 更新内部状态。从链表头部吐出pumpSize个节点，并更新初始位置和花费。
    /// @param pumpSize 吐出的节点数量
    /// @param recLst 用于接收节点的链表
    void pumpPlan(int pumpSize, std::list<ActionNode>& recLst) {
        auto lastIt = std::next(this->actionNodes.begin(), pumpSize);
        this->actionNodes.front().endPos = (*lastIt).endPos;
        this->actionNodes.front().endTokens = (*lastIt).endTokens;
        if ((*lastIt).action.action == READ || (*lastIt).action.action == VREAD) {
            this->actionNodes.front().action.aheadRead = (*lastIt).action.aheadRead;
        }
        else {
            this->actionNodes.front().action.aheadRead = 0;
        }
        lastIt++;
        recLst.splice(recLst.end(), this->actionNodes,
            std::next(this->actionNodes.begin(), 1), lastIt);
    }
    /// @brief 执行当前时间步内的行动
    /// @param handledOperation 用来接收执行的行动
    /// @param completedRead 用来接收完成的读操作
    void excutePresentTimeStep(std::vector<HeadOperator>* handledOperation, std::vector<int>* completedRead) {
        auto lastIt = actionNodes.begin();
        auto nextIt = std::next(lastIt);
        LOG_PLANNERN(diskId) << "\nexcute on plan:"<<actionNodes;
        while (nextIt != actionNodes.end() && Watch::toTimeStep((*nextIt).endTokens) <= Watch::getTime()) {
            #ifdef ENABLE_ACCELERATION
            lastIt++;
            nextIt++;
            handledOperation->push_back((*lastIt).action);
            if((*lastIt).action.action == READ){
                completedRead->push_back(((*lastIt).endPos-1+spaceSize)%spaceSize);
            }
            #else
            LOG_PLANNERN(diskId) << "before time:" << Watch::toTimeStep((*nextIt).endTokens) << ",end time:" << Watch::getTime();
            lastIt++;
            nextIt++;
            assert((*lastIt).action.param >= 0);
            
            if (!head->beginAction((*lastIt).action)) {
                throw std::logic_error("can not begin?!");
            }
            if (!head->completeAction(handledOperation, completedRead)) {
                throw std::logic_error("can not complete?!");
            }
            
            LOG_PLANNERN(diskId) << "excute operation " << *lastIt;
            #endif
        }
        if (lastIt == actionNodes.begin()) {
            return;
        }
        //lastIt是最后一个执行的行动。
        this->actionNodes.front().endPos = (*lastIt).endPos;
        this->actionNodes.front().endTokens = (*lastIt).endTokens;
        #ifdef ENABLE_ACCELERATION
        int presentTokens = Watch::getTime()*G - (*lastIt).endTokens;
        #else
        int presentTokens = Watch::getTime()*G - (*lastIt).endTokens;
        assert(presentTokens == head->presentTokens);
        #endif
        if (nextIt != actionNodes.end() && (*nextIt).action.action == PASS && presentTokens > 0) {
            HeadOperator operation = { PASS, presentTokens };
            this->actionNodes.front().endTokens += operation.passTimes;
            this->actionNodes.front().endPos += operation.passTimes;
            this->actionNodes.front().endPos %= spaceSize;
            this->actionNodes.front().action.aheadRead = 0;
            nextIt->action.passTimes -= operation.passTimes;

            #ifdef ENABLE_ACCELERATION
            handledOperation->push_back(operation);
            #else
            head->beginAction(operation);
            head->completeAction(handledOperation, completedRead);
            #endif
        }
        else {
            if ((*lastIt).action.action == READ || (*lastIt).action.action == VREAD) {
                this->actionNodes.front().action.aheadRead = (*lastIt).action.aheadRead + 1;
            }
            else {
                this->actionNodes.front().action.aheadRead = 0;
            }
        }
        actionNodes.erase(std::next(actionNodes.begin()), nextIt);
        LOG_PLANNERN(diskId) << "over excute, plan: " << actionNodes << ",read:"<<*completedRead<<",handled:"<<*handledOperation<<",headpos:"<<head->headPos;
    }
    const Disk* getDisk() const;
    const DiskHead* getHead() const {return this->head;}

    //返回最小距离处的前一个节点。该节点执行完毕后即为最小距离。
    std::list<ActionNode>::iterator getShortestDistance(int target, int* getDist) {
        auto aheadIt = this->actionNodes.begin();
        *getDist = getDistance(target, (*aheadIt).endPos, this->spaceSize);
        int pos; int dist;
        for (auto it = std::next(aheadIt, 1); it != this->actionNodes.end(); it++) {
            pos = (*it).endPos;
            //找一个最小的位置。或者找一个在jump之前的位置。注意jump之后就应该plan这个read，防止多个jump到一个地方。
            dist = getDistance(target, pos, this->spaceSize);
            if (dist < *getDist) {
                *getDist = dist;
                aheadIt = it;//在谁之后插入。
            }
        }
        return aheadIt;
    }
    
    void test_syncWithHeadTest() const;
    void test_nodeContinuousTest() const;
    void test_branchAndMainConsistency() const {

    }

    ~HeadPlanner() {
        for (auto it = this->readBranches.begin(); it != this->readBranches.end(); it++) {
            delete (*it).second.second;//删除除了当前分支外的所有分支
        }
    }
};




#endif