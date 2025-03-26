#ifndef HEADPLANNER_H
#define HEADPLANNER_H

#include "disk.h"

struct ActionNode{
    HeadOperator action;
    int endPos;
    int endTokens;
};

inline LogStream& operator<<(LogStream& s, ActionNode& actionNode){
    s << "[" << actionNode.action << 
        ",finishPos:"<< actionNode.endPos <<
        ",endTokens:" << actionNode.endTokens  <<"]";
    return s;
}

class HeadPlanner{//全局plan，其中的tokens为全局tokens。
private:
    friend LogStream& operator<<(LogStream& s, HeadPlanner& headPlanner);
    int spaceSize;
    Disk* disk;//保持对disk的引用。
    //应该存入绝对tokens数。
    std::map<int, std::pair<std::list<ActionNode>::iterator, HeadPlanner*>> readBranches = {};

    //分支行动。分别为：增加读的位置/创建分支的位置/分支策略。
    int diskId;
    //三元组<HeadOperator, int, int>:磁头操作/操作结束位置/操作结束时间。
    std::list<ActionNode> actionNodes = {};//第零个是起始node。
    

    // HeadPlanner(int readConsume, int headpos, int spacesize):
    //     nextReadConsume(readConsume), orgHeadPos(headpos), spaceSize(spacesize){}
public:
    HeadPlanner(Disk* disk):
        spaceSize(disk->head.spaceSize),
        diskId(disk->diskId),
        disk(disk)
    {
        if(disk->head.tokensOffset!=0){
            throw std::logic_error("don't initialize headPlanner by head with action incompleted.");
        }
        actionNodes.push_back({{START, getAheadReadTimes(disk->head.readConsume)}, 
                                disk->head.headPos, 
                                G - disk->head.presentTokens});
        assert(G - disk->head.presentTokens == 0);
    }
    //用一个actionNode来创建分支。
    HeadPlanner(Disk* disk, const ActionNode& actionNode):
        spaceSize(disk->head.spaceSize),
        diskId(disk->diskId),
        disk(disk)
    {
        int aheadRead = 0;
        if(actionNode.action.action == READ){
            aheadRead = actionNode.action.aheadRead + 1;
        }else if(actionNode.action.action == START){
            aheadRead = actionNode.action.aheadRead;
        }
        actionNodes.push_back({{START, aheadRead}, 
                                actionNode.endPos, 
                                actionNode.endTokens});
        if(disk->head.tokensOffset!=0){
            throw std::logic_error("don't initialize headPlanner by head with action incompleted.");
        }
        //actionNodes.push_back({{START, 0}, disk->head.headPos, G - disk->head.presentTokens});
    }
    //不要用还未完成行动的head来初始化HeadActionsInfo！
    // 将操作序列附加到现有操作操作序列中，并更新所有操作的累积花费和读操作状态。
    // @param actions 要附加的操作序列。
    const ActionNode& getLastActionNode(){
        return actionNodes.back();
    }
    const std::list<ActionNode>& getActionNodes(){
        return this->actionNodes;
    }
    const int& getDiskId(){return this->diskId;}
    void appendActions(const std::vector<HeadOperator>& actions){
        for(int i=0;i<actions.size();i++){
            auto operation = actions[i];
            appendAction(operation);
        }
    }
    void appendActions(const std::list<HeadOperator>& actions){
        for(auto it=actions.begin();it!=actions.end();it++){
            auto operation = *it;
            appendAction(operation);
        }
    }
    int getAheadReadAfter(const ActionNode& lastNode){
        int aheadRead;
        if(lastNode.action.action == READ){
            aheadRead = lastNode.action.aheadRead + 1;
        }else if(lastNode.action.action == START){
            aheadRead = lastNode.action.aheadRead;
        }
        return aheadRead;
    }
    
    //单纯地加入一个行动，并且计算这个行动的新位置、新代价。
    void appendAction(const HeadOperator& operation, bool ignoreAheadRead = false){
        if(this->readBranches.size()){
            std::logic_error("should not append when has branch");
        }
        auto action = operation.action;
        if(action == READ){//对于READ操作
            int aheadRead = getAheadReadAfter(getLastActionNode());
            
            if(operation.aheadRead != -1 && (!ignoreAheadRead)){
                assert(operation.aheadRead == aheadRead);
            }
            this->pushBackActionNode({READ, aheadRead}, 1, true, 
                                    getReadConsumeAfterN(aheadRead), false);
        }else if(action == PASS){
            //PASS不需要细分操作数，因为不需要判断某个单元是否有某个对象在赶往，
            // 只需要知道某个单元什么时候会被读取即可。
            this->pushBackActionNode(operation, operation.passTimes, true, 
                                    operation.passTimes, true);
        }else if(action == JUMP){
            if(operation.jumpTo<0 || operation.jumpTo>=spaceSize){
                throw std::out_of_range("action JUMP jumps out of range");
            }
            this->pushBackActionNode(operation, operation.jumpTo, false, G, false);
        }
        else {
            throw std::logic_error("action wrong!");
        }
        
    }
    //该函数具有规划的性质，会判断如何到达reqUnit
    void appendMoveTo(int reqUnit){
        if(this->readBranches.size()){
            std::logic_error("should not append when has branch");
        }
        LOG_ACTIONSN(this->diskId) << "plan to reqUnit:" << reqUnit;
        int distance = disk->getDistance(reqUnit, this->getLastActionNode().endPos);//choseRep已经算过了一遍，这里再算一遍？！
        
        if(distance){
            int leftTokens = G - this->getLastActionNode().endTokens%G;
            if(distance <= leftTokens){
                //如果可以直接移动到目标位置。
                this->appendAction(HeadOperator{PASS, distance});
                //也可以试试能不能连读过去（节省时间），这取决于下面有多少单元，能否节省总开销。
            }else if(leftTokens == G){
                //当前还有G个Token，即上一时间步的操作没有延续到当前时间步。可以直接跳过去。
                this->appendAction(HeadOperator{JUMP, reqUnit});
            }else if(leftTokens + G > distance){
                //如果当前时间步的令牌数不够跳，且这个时间步+下个时间步能够移动到目标位置，
                //那么直接向目标位置移动
                this->appendAction(HeadOperator{PASS, distance});
            }else{
                //最早也得下两个回合才能到达读位置。直接放入跳。
                this->appendAction(HeadOperator{JUMP, reqUnit});
            }
        }else{
            return;//就在当前位置。
        }
    }

    //规划性质：插入读操作，策略为选择最近插入位置。
    void insertReadAsBranch(int unitPos, int* getReadOverTokens, int* getScoreLoss){
        auto aftIt = this->actionNodes.begin();
        int pstDist = getDistance(unitPos, (*aftIt).endPos);
        if(this->actionNodes.size()>1){
            int pos;int dist;
            for(auto it = this->actionNodes.begin();it != this->actionNodes.end();it++){
                pos = (*it).endPos;
                //找一个最小的位置。或者找一个在jump之前的位置。注意jump之后就应该plan这个read，防止多个jump到一个地方。
                dist = getDistance(unitPos, pos);
                if(dist<pstDist){
                    pstDist = dist;
                    aftIt = it;//在谁之后插入。
                }
            }
        }
        
        HeadPlanner* branch = new HeadPlanner(disk, *(aftIt));
        readBranches.insert({unitPos, {aftIt, branch}});
        branch->appendMoveTo(unitPos);
        branch->appendAction({READ, -1});
        *getReadOverTokens = branch->getLastActionNode().endTokens;
        aftIt ++;
        auto nextAction = (*aftIt).action.action;
        if(aftIt != this->actionNodes.end()){
            if(nextAction==PASS){
                branch->appendMoveTo((*aftIt).endPos);
                aftIt ++;
            }else if(nextAction==READ){
                //如果是READ，那么上一个endPos距离unitPos最近且为下一个位置减1,那么上一个endPos就是uniPos。
                assert(branch->actionNodes.front().endPos==unitPos);
                //但是它既然停在了上一个位置，又没有读。就很奇怪了。不读为什么要停在那。
                throw std::logic_error("strange!!!");
            }//如果是JUMP，直接appendAction即可。
        }
        
        std::vector<Object*> relatedObjects = {};
        while(aftIt != this->actionNodes.end()){
            branch->appendAction((*aftIt).action, true);
            const ActionNode& newNode = branch->getLastActionNode();
            if(getScoreLoss != nullptr && newNode.action.action == READ){
                auto unitInfo = disk->getUnitInfo(newNode.endPos-1);
                Object* obj = sObjectsPtr[unitInfo.objId];
                obj->virBranchPlan(unitInfo.untId, newNode.endTokens/G, false);
                relatedObjects.push_back(obj);
            }
        }
        if(getScoreLoss != nullptr){
            for(int i=0;i<relatedObjects.size();i++){
                *getScoreLoss += relatedObjects[i]->getScoreLoss();
            }
        }
        //向后移动看看能不能改进总价值?
    }
    //用分支修改主干
    void mergeReadBranch(int unitPos){
        auto branchInfo = this->readBranches.at(unitPos);
        actionNodes.splice(branchInfo.first, branchInfo.second->actionNodes,
            std::next(branchInfo.second->actionNodes.begin(),1));//把branch的分支移动过来。
        this->readBranches.erase(unitPos);
        for(auto it = this->readBranches.begin();it!=this->readBranches.end();it++){
            delete (*it).second.second;//删除除了当前分支外的所有分支
        }
        this->readBranches.clear();

        this->readBranches = std::move(branchInfo.second->readBranches);//把branch的分支移动到当前。
        delete branchInfo.second;
    }
    //删除分支
    void dropReadBranch(int unitPos){
        auto branchInfo = this->readBranches.at(unitPos);
        this->readBranches.erase(unitPos);
        delete branchInfo.second;
    }

    void iteratorToNextRead(std::list<ActionNode>::iterator& it){
        it ++;
        while((*it).action.action != READ){
            it ++;
        }
    }

    int getDistance(int target, int from){
        return (target - from + spaceSize)%spaceSize;
    }

    void pushBackActionNode(const HeadOperator& operation, int posValue, bool isOffset, int costValue, bool divisible){
        auto lastNode = getLastActionNode();
        int newCost = calNewTokensCost(lastNode.endTokens, costValue, divisible);
        int newPos = calNewHeadPos(lastNode.endPos, posValue, isOffset);
        actionNodes.push_back({operation, newPos, newCost});
    }
    
    void test_syncWithHeadTest() const {
        if(this->actionNodes.front().endPos != this->disk->head.headPos){
            std::logic_error("the planner status not sync with head!!!");
        }
    }

    int calNewTokensCost(int lastTokenCost, int value, bool divisible) {
        if (divisible) {
            return lastTokenCost + value;
        }
        else {
            int tolTokens = lastTokenCost;
            if ((tolTokens + value - 1) / G == tolTokens / G + 1) {//如果操作跨回合了。
                tolTokens = (tolTokens / G + 1) * G;//跳到回合开始处。
            }
            tolTokens += value;
            return tolTokens;
        }
    }
    int calNewHeadPos(int lastHeadPos, int value, bool isOffset){
        if (isOffset) {
            return (lastHeadPos + value) % spaceSize;
        }
        else {
            return value;
        }
    }
    // int getTokenCost(){return tokenCost;}
    // int getTimeCost(){return timeCost;}
    // int getNextReadConsume(){return nextReadConsume;}
    // std::vector<std::pair<HeadAction, int>> getActions(){return actions;}

    //更新内部状态
    
    /// @brief 更新内部状态。从链表头部吐出pumpSize个节点，并更新初始位置和花费。
    /// @param pumpSize 吐出的节点数量
    /// @param recLst 用于接收节点的链表
    void pumpPlan(int pumpSize, std::list<ActionNode>& recLst){
        auto lastIt = std::next(this->actionNodes.begin(), pumpSize);
        this->actionNodes.front().endPos = (*lastIt).endPos;
        this->actionNodes.front().endTokens = (*lastIt).endTokens;
        if((*lastIt).action.action==READ){
            this->actionNodes.front().action.aheadRead = (*lastIt).action.aheadRead;
        }else{
            this->actionNodes.front().action.aheadRead = 0;
        }
        lastIt++;
        recLst.splice(recLst.end(), this->actionNodes, 
            std::next(this->actionNodes.begin(), 1), lastIt);
    }

    ~HeadPlanner(){
        for(auto it = this->readBranches.begin();it!=this->readBranches.end();it++){
            delete (*it).second.second;//删除除了当前分支外的所有分支
        }
    }
};

inline LogStream& operator<<(LogStream& s, HeadPlanner& headPlanner){
    s << "\n";
    s << "diskId:" << headPlanner.diskId << " ";
    s << "spaceSize:" << headPlanner.spaceSize << "\n";
    s << "(actions,topos,endTokens):\n{";
    for(auto it = headPlanner.actionNodes.begin();it != headPlanner.actionNodes.end();it++){
        s <<""<< *it <<", ";
    }
    s << "}\n";
    return s;
}


#endif