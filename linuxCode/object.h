#if !defined(OBJECT_H)
#define OBJECT_H

#include <cstring>
#include <algorithm>
#include <assert.h>
#include "global.h"
#include "watch.h"

class Object;
LogStream& operator<<(LogStream& s, const Object& obj);

class Request;
extern Object* deletedObject;//作为一个被删除/不存在的对象。
extern Request* deletedRequest;
extern Object* sObjectsPtr[MAX_OBJECT_NUM];
extern Request* requestsPtr[MAX_REQUEST_NUM];
extern std::vector<Object*> requestedObjects;//通过该链表可以更新价值。并且通过该链表排序。对象被删除时也要更新。
extern int overtimeReqTop;//指向的req刚好未过期，或者为nullptr
extern int phaseTwoTop;//或者为nullptr，或者指向的上一个位置的req在下一帧正式进入phase2

class Request {
public:
    Request(int reqId, int objId, int size){
        this->objId = objId;
        this->reqId = reqId;
        this->createdTime = Watch::getTime();
        this->unitFlag = (bool*)malloc(size*sizeof(bool));
        for(int i=0;i<size;i++){
            this->unitFlag[i] = false;
        }
        this->is_done = -size + 1;
    }
    bool commitUnit(int unit){
        if(!this->unitFlag[unit]){
            this->unitFlag[unit] = true;
            is_done += 1;
            return true;
        }
        return false;
    }
    void calScore(int size, int* getScore, int* getEdgeValue){
        int offset = Watch::getTime() - this->createdTime;
        if(offset<=PHASE_ONE_TIME){
            *getScore = SCORE_FACTOR(size)*(START_SCORE - PHASE_ONE_EDGE*offset);
        }else if(offset<EXTRA_TIME){
            //第二阶段
            *getScore = SCORE_FACTOR(size)*(START_SCORE + (PHASE_TWO_EDGE-PHASE_ONE_EDGE)*PHASE_ONE_TIME - PHASE_TWO_EDGE*offset);
        }
        //由于边缘是下个时间片开始时减少的分数，而非这个时间片减少的分数。
        if (offset < PHASE_ONE_TIME) {
            *getEdgeValue = SCORE_FACTOR(size) * PHASE_ONE_EDGE;
        }
        else if (offset < EXTRA_TIME) {
            //第二阶段
            *getScore = SCORE_FACTOR(size) * (START_SCORE + (PHASE_TWO_EDGE - PHASE_ONE_EDGE) * PHASE_ONE_TIME - PHASE_TWO_EDGE * offset);
            *getEdgeValue = SCORE_FACTOR(size) * PHASE_TWO_EDGE;
        }
    }
    // int getEdgeValue(){
    //     int edgeValue = ((Watch::getTime() - this->createdTime)<=10)?PHASE_ONE_EDGE:PHASE_TWO_EDGE;
    //     return edgeValue;
    // }
    int objId;
    int reqId;
    bool* unitFlag;
    
    int is_done;
    int createdTime;
};

//inline void deleteRequest(int id);
inline void deleteRequest(int id) {
    LOG_REQUEST << "delete request " << id << " for object " << requestsPtr[id]->objId;
    Request* request = requestsPtr[id];
    if (request == deletedRequest) {
        assert(false);
    }
    delete request;
    requestsPtr[id] = deletedRequest;
}

class Object{//由object来负责request的管理（创建/删除）
public:
    const int size;//大小
    const int tag;
    const int objId;
    

    //存储时由磁盘设置。
    int replica[REP_NUM];//副本所在磁盘
    bool scatterStore[REP_NUM];
    int* unitOnDisk[REP_NUM];//相应位置的单元存储在磁盘上的位置

    //规划时设置（优化方向：采用专门的对象通过注入方式临时分配规划内存。
    //基本规划信息
    int* unitReqNum;//相应位置单元上的剩余请求数
    int* unitReqNumOrder;//哪个单元的req最大，哪个单元就在前面。
    int* planReqUnit;//相应位置的单元是否已经被规划,如果被规划，记录分配的磁盘，否则置-1（防止其它磁盘再读）
    int* planReqHead;//记录规划分配的磁头
    int* planReqTime;//相应位置的单元被规划完成的时间,记录该单元完成读请求的时间，否则置-1（防止其它磁盘再读）
    //每次规划使用
    int* planBuffer;//大小为size//给branch用于临时比较代价的buffer。使用完需要置负一。也可以给连读策略用。判断是否已经被规划。
    //价值（优化方向：当前直接使用下一时刻的边缘，动态分配后可以保存多个时刻的边缘。
    int score = 0;//当前请求剩余的总分数
    int edgeValue = 0;//下一个时间步分数降低多少
    int* coScore;
    int* coEdgeValue;
    std::list<Request*> objRequests = {};//sorted by time//不包含已完成请求。

    Object(int id, int size, int tag):objId(id),tag(tag),size(size){
        //this->unitReqNum = (int*)malloc((REP_NUM+3)*size*sizeof(int));//malloc分配0空间是合法操作。
        //分配内存
        this->unitReqNum = (int*)malloc(size*(REP_NUM+8)*sizeof(int));
        for(int i=0;i<REP_NUM;i++){
            this->unitOnDisk[i] = this->unitReqNum + size*(i+1);
        }
        this->planReqUnit = this->unitReqNum + size*(REP_NUM+1);
        this->planReqHead = this->unitReqNum + size*(REP_NUM+7);
        this->planReqTime = this->unitReqNum + size*(REP_NUM+2);
        this->unitReqNumOrder = this->unitReqNum + size*(REP_NUM+3);//在commitunit时维护。
        
        this->coScore = this->unitReqNum + size*(REP_NUM+4);
        this->coEdgeValue = this->unitReqNum + size*(REP_NUM+5);

        this->planBuffer = this->unitReqNum + size*(REP_NUM+6);

        //初始化
        for(int i=0;i<size;i++){
            this->unitReqNum[i] = 0;
            this->planReqUnit[i] = -1;
            this->planReqHead[i] = -1;
            this->planReqTime[i] = -1;
            this->planBuffer[i] = -1;
            this->unitReqNumOrder[i] = i;
            this->coScore[i] = 0;
            this->coEdgeValue[i] = 0;
        }

        for(int i=0;i<REP_NUM;i++){
            this->scatterStore[i] = false;
        }
    }

    // bool isPlaned(int unitId){
    //     if(this->planReqUnit[unitId]>=0){
    //         return true;
    //     }
    //     return false;
    // }
    bool allPlaned() const {
        bool flag = true;
        for(int i=0;i<size;i++){
            //如果存在对i的请求且该请求还未规划
            if(this->unitReqNum[i] && planReqTime[i]<Watch::getTime()){
                flag = false;
            }
        }
        return flag;
    }
    bool isPlaned(int unitId) const {
        if(this->planReqUnit[unitId]>=0
            && planReqTime[unitId]>=Watch::getTime()){
            return true;
        }
        return false;
    }
    bool isRequested(int unitId) {
        return this->unitReqNum[unitId] > 0;
    }
    int planedBy(int unitId){
        if(this->planReqUnit[unitId]>=0){
            return this->planReqUnit[unitId];
        }else{
            return -1;
        }
    }
    void plan(int unitId, int diskId, int headId){
        this->planReqUnit[unitId] = diskId;
        this->planReqHead[unitId] = headId;
        this->planReqTime[unitId] = Watch::getTime();
    }
    void plan(int unitId, int diskId, int headId, int timeRequired, bool isOffset){
        if(isOffset){
            this->planReqTime[unitId] = timeRequired + Watch::getTime();
        }else{
            this->planReqTime[unitId] = timeRequired;
        }
        this->planReqUnit[unitId] = diskId;
        this->planReqHead[unitId] = headId;
    }
    void test_plan(int unitId, int diskId, int headId, int timeRequired, bool isOffset){
        if(isOffset){
            assert(this->planReqTime[unitId] == timeRequired + Watch::getTime());
        }else{
            assert(this->planReqTime[unitId] == timeRequired);
        }
        assert(this->planReqUnit[unitId] = diskId);
        assert(this->planReqHead[unitId] = headId);
    }
    void clearPlaned(int unitId){
        this->planReqUnit[unitId] = -1;
        this->planReqHead[unitId] = -1;
        this->planReqTime[unitId] = 0;
    }
    
    //用于planner创建分支比较花销。use planBuffer
    void virBranchPlan(int unitId, int timeRequired, bool isOffset){
        if(this->planBuffer[unitId] > 0){
            throw std::logic_error("should has been cleared!!");
        }
        if(isOffset){
            this->planBuffer[unitId] = timeRequired + Watch::getTime();
        }else{
            this->planBuffer[unitId] = timeRequired;
        }
    }
    int getScoreLoss(){//会自动清除virPlanReqTime
        int delay;
        int accEdgeLoss = 0;
        int maxAheadPlanReqTime = 0;
        for(int i=0;i<this->size;i++){
            int unitId = this->unitReqNumOrder[i];
            if(this->unitReqNum[unitId]==0){
                continue;
            }
            if(this->unitReqNum[unitId]<0){
                throw std::logic_error("num less than zero!!!");
            }
            //规划单元时，有可能遇见当前单元的对象。这时候只需要计算到已规划的内容即可
            if (this->planReqTime[unitId] <= 0) { 
                break;
            }//遇见了当前单元的对象
            //第(i+1)个协同的最早完成时间。
            maxAheadPlanReqTime = std::max<int>(maxAheadPlanReqTime, this->planReqTime[unitId]);
            
            if(this->planBuffer[unitId] > 0){
                LOG_OBJECT << *this;
                assert(this->planReqTime[unitId] >= Watch::getTime());
                delay = std::max<int>(this->planBuffer[unitId] - maxAheadPlanReqTime, 0);
                assert(delay>=0);//目前保证这一点，后续可以扩展为负数。
                accEdgeLoss += delay * this->coEdgeValue[i];
            }else{
                //没有延迟。按原计划进行。没有损失。
            }
        }

        for(int i=0;i<this->size;i++){
            this->planBuffer[i] = -1;
        }
        
        return accEdgeLoss;
    }
    //用于连读比较读取分数。use planBuffer
    void setUnitCompleteStepInPlanBuffer(int unitId, int value){
        this->planBuffer[unitId] = value;
    }
    
    //无法正确计算分数。会受到原planBuffer的影响。
    //会直接把分数累加到输入量上。
    void calScoreAndClearPlanBuffer(int* getScore, int* getEdge, int* getReqNum){
        // *getScore = 0;
        // *getEdge = 0;
        int earliestGetTime = 0;
        LOG_OBJECT << *this;
        for(int i=0;i<size;i++){
            int unitId = this->unitReqNumOrder[i];
            if(this->planBuffer[unitId] >= Watch::getTime()){
                earliestGetTime = std::max(this->planBuffer[unitId], earliestGetTime);
                //*getScore += 1+std::max(this->coScore[unitId] - this->coEdgeValue[unitId]*(earliestGetTime - Watch::getTime()), 0);
                for (int j = i; j < size; j++) {
                    *getScore += this->coScore[j]/(j+1);
                }
                *getEdge += this->coEdgeValue[unitId];//一个简单的Edge，没有考虑到具体时间。
                *getReqNum += this->unitReqNum[unitId];
                this->planBuffer[unitId] = -1;//计算完了这一阶段的分数，把buffer归-1.
            }else if(this->planReqTime[unitId] >= Watch::getTime()){
                earliestGetTime = std::max(this->planReqTime[unitId], earliestGetTime);
            }
        }
    }
    
    //会直接把分数累加到输入量上。
    void calUnitScoreAndEdge(int unitId, int* getScore, int* getEdge){//会用四倍！
        for(int i=this->size-1;i>=0;i--){
            *getScore += this->coScore[i]/(i+1);
            *getEdge += this->coEdgeValue[i]/(i+1);
            if(this->unitReqNumOrder[i] == unitId){
                return;
            }
        }
    }
    //确认单元之后，需要在reqSpace中删除其它副本。
    /// @brief commit a unit which has been read.
    /// @param unitId unit order in object
    /// @param doneRequestIds requests completed by this unit will be push_back to this vector.
    void commitUnit(int unitId, std::vector<int>* doneRequestIds){
        if(unitId>=size){
            throw std::out_of_range("obj unit out of range!");
        }
        
        //删除请求，更新完成请求。更新分数。
        auto it=objRequests.begin();
        while(it!=objRequests.end()){
            Request* request = *it;
            bool freshed = request->commitUnit(unitId);
            int getScore;int getEdge;
            if (Watch::getTime() == 394 && objId == 897) {
                int test = 0;
            }
            if(request->is_done > 0){//只要有请求done了，被完成的一定是剩余单元数最大的请求。
                assert(unitId==unitReqNumOrder[0]);
                LOG_REQUEST<<"request "<<request->reqId<<" done";
                doneRequestIds->push_back(request->reqId);

                request->calScore(this->size, &getScore, &getEdge);
                flowDownScoreAndEdge(0, getScore, getEdge);
                deleteRequest(request->reqId);
                it = objRequests.erase(it);//it会指向被删除元素的下一个元素
            }else if(freshed){
                //更新协同分数值。
                int coValue = -request->is_done+1;//协同值，也即剩余的请求单元数。
                request->calScore(this->size, &getScore, &getEdge);
                //这里只是协同分数的流动。协同分数还需要在别的地方更新！！！
                flowDownScoreAndEdge(coValue, getScore, getEdge);
                it++;
            }
            else {
                it++;
            }
        }

        //把请求该单元的请求数、该单元分配给的磁盘都重置。
        this->unitReqNum[unitId] = 0;
        this->clearPlaned(unitId);
        //更新请求数顺序
        for(int i=0;i<size;i++){
            if(unitReqNumOrder[i] == unitId){
                memmove(unitReqNumOrder+i, unitReqNumOrder+i+1, sizeof(int)*(size-i-1));
                unitReqNumOrder[size-1] = unitId;
                break;
            }
        }
    }
    std::vector<int> createRequest(int reqId){//返回新的单元请求。
        Request* request = new Request(reqId, objId, this->size);
        std::vector<int> newReqUnit = {};
        LOG_REQUEST << "create request "<<reqId <<" for object "<<objId;
        
        this->objRequests.push_back(request);
        requestsPtr[reqId] = request;
        for(int i=0;i<size;i++){
            if(this->unitReqNum[i] == 0){
                newReqUnit.push_back(i);
            }
            this->unitReqNum[i] += 1;
        }

        //更新分数和边缘价值
        pushRequestScoreAndEdge();

        return newReqUnit;
    }

    /// @brief 提前一定时间步预判并删除超时请求，更新分数
    /// @param reqId 删除的请求Id
    /// @param preStep 提前的时间步数量
    /// @return 返回取消的请求单元(reqUnit)在obj中的位置(unitId)
    std::vector<int> dropOvertimeRequest(int reqId, vector<int>& overtimeReqs){//超时删除的请求。
        //更新obj内部的单元请求数，更新disk内的请求单元链表。
        std::vector<int> overtimeReqUnits = {};
        auto req = requestsPtr[reqId];
        for(int i=0;i<this->size;i++){//第几个unit
            if(!req->unitFlag[i]){//如果该请求的该单元还未完成。
                this->unitReqNum[i] -= 1;
                if(this->unitReqNum[i]==0){
                    overtimeReqUnits.push_back(i);
                }
            }
        }
        //删除请求.该请求一定处于最后方。因为之前的请求都完成了。
        if((*this->objRequests.begin())->reqId == reqId){
            //overtimeRequests.push_back(reqId);
            overtimeReqs.push_back(reqId);
            this->objRequests.erase(this->objRequests.begin());
        }else{
            throw std::logic_error("dropped request shold be the first request!");
        }
        //更新分数
        int getScore = 0; int getEdge = 0;
        req->calScore(this->size, &getScore, &getEdge);
        freshOvertimeReq(1-req->is_done, getScore, getEdge);
        //这可不是协作值！这是因为请求过期而请求总数为0的单元数。assert(overtimeReqUnits.size() == 1-req->is_done);//未完成的单元数，即协作值。
        deleteRequest(reqId);
        
        return overtimeReqUnits;
    }
    bool hasRequest(){
        if(this->objRequests.size()>0){
            return true;
        }
        return false;
    }
    //没有request或者只有超时的request。
    bool hasValidRequest(){
        if((!this->objRequests.size()) ||
                this->objRequests.back()->createdTime == Watch::getTime()-105){
            return false;
        }else{
            return true;
        }
    }
    int getReqUnitSize(){
        int reqUnitSize = 0;
        for(int i=0;i<this->size;i++){
            if(this->unitReqNum[i]>0) {
                reqUnitSize += 1;
            }
        }
        return reqUnitSize;
    }
    
    //score相关。更新score。
    void pushRequestScoreAndEdge(){
        this->score += START_SCORE*SCORE_FACTOR(size);//同一个对象最多支持2万个请求的计分。超过的话有可能分数值溢出。
        this->edgeValue += PHASE_ONE_EDGE*SCORE_FACTOR(size);//开始时每秒减5分，后续阶段由worker更新。
        this->coScore[this->size-1] += START_SCORE*SCORE_FACTOR(size);
        this->coEdgeValue[this->size-1] += PHASE_ONE_EDGE*SCORE_FACTOR(size);
    }
    void flowDownScoreAndEdge(int coValue, int score, int edge){
        assert(coValue>=0 && coValue < size);
        if(coValue==0){
            this->edgeValue -= edge;
            this->score -= score;
            this->coEdgeValue[0] -= edge;
            this->coScore[0] -= score;
        }else{
            this->coScore[coValue] -= score;//协同值比索引大1.
            this->coEdgeValue[coValue] -= edge;
            this->coScore[coValue-1] += score;
            this->coEdgeValue[coValue-1] += edge;
        }
    }
    void clockScore(){
        this->score -= this->edgeValue;
        for(int i=0;i<this->size;i++){
            this->coScore[i] -= this->coEdgeValue[i];
        }
    }
    void freshPhaseTwoEdge(int coValue){
        if (objId == 3110) {
            int test = 0;
            int test2 = test+3;
        }
        this->edgeValue += (PHASE_TWO_EDGE - PHASE_ONE_EDGE)*SCORE_FACTOR(size);
        this->coEdgeValue[coValue-1] += (PHASE_TWO_EDGE - PHASE_ONE_EDGE)*SCORE_FACTOR(size);
    }
    void freshOvertimeEdge(int coValue){
        this->edgeValue -= PHASE_TWO_EDGE*SCORE_FACTOR(size);
        this->coEdgeValue[coValue-1] -= PHASE_TWO_EDGE*SCORE_FACTOR(size);
    }
    void freshOvertimeReq(int coValue, int score, int edge){
        this->edgeValue -= edge;
        this->score -= score;
        this->coEdgeValue[coValue-1] -= edge;
        this->coScore[coValue-1] -= score;
    }
    
    
    //just for test. after overtime request removed.
    void test_validRequestsTest() const {
        for(auto it = objRequests.begin();it!=objRequests.end();it++){
            auto request = *it;
            if(request == deletedRequest){
                throw std::logic_error("deleted request!!!");
            }
            if(Watch::getTime() - request->createdTime == 105){
                throw std::logic_error("overtime request!!!");
            }
        }
    }

    //注意：如果一次取磁盘能满足多个请求，那么可以取得较大效益。
    //可以以Object来组织请求。
    ~Object(){
        free(this->unitReqNum);
        
        //删除属于该对象的所有请求。
        for(auto it = objRequests.begin();it!=objRequests.end();it++){
            deleteRequest((*it)->reqId);//request的生命周期应该由object来掌管吗？
        }
    }
};

//inline void deleteObject(int id);
//inline Object* createObject(int id, int size, int tag);

inline Object* createObject(int id, int size, int tag) {
    LOG_OBJECT << "create object " << id << " size " << size << " tag " << tag;
    Object* object = new Object(id, size, tag);
    sObjectsPtr[id] = object;
    return object;
}
inline void deleteObject(int id) {

    Object* object = sObjectsPtr[id];
    LOG_OBJECT << "delete object " << object->objId;
    if (object == deletedObject) {
        LOG_OBJECT << "object size " << object->size << "object id " << object->objId;
        throw std::logic_error("object has been deleted!");
    }
    LOG_OBJECT << " ";
    sObjectsPtr[id] = deletedObject;

    delete object;

    LOG_OBJECT << "deleted over ";
}


#endif



