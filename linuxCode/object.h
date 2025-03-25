#if !defined(OBJECT_H)
#define OBJECT_H
#include "global.h"
#include "watch.h"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <assert.h>


#define LOG_REQUEST LOG_FILE("request")
#define LOG_OBJECT LOG_FILE("object")
class Request;
class Object;
extern Object deletedObject;//作为一个被删除/不存在的对象。
extern Request deletedRequest;
extern Object* sObjectsPtr[MAX_OBJECT_NUM];
extern Request* requestsPtr[MAX_REQUEST_NUM];
extern std::list<Object*> requestedObjects;//通过该链表可以更新价值。并且通过该链表排序。对象被删除时也要更新。
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
    void commitUnit(int unit){
        if(!this->unitFlag[unit]){
            this->unitFlag[unit] = true;
            is_done += 1;
        }
    }
    int objId;
    int reqId;
    bool* unitFlag;
    
    int is_done;
    int createdTime;
};


inline void deleteRequest(int id){
    LOG_REQUEST << "delete request "<<id <<" for object "<<requestsPtr[id]->objId;
    Request* request = requestsPtr[id];
    if(request == &deletedRequest){
        assert(false);
    }
    delete request;
    requestsPtr[id] = &deletedRequest;
}


class Object{//由object来负责request的管理（创建/删除）
public:
    const int size;//大小
    const int tag;
    const int objId;

    //存储时由磁盘设置。
    int replica[REP_NUM];//副本所在磁盘
    int* unitOnDisk[REP_NUM];//相应位置的单元存储在磁盘上的位置
    //规划时设置
    int* unitReqNum;//相应位置单元上的剩余请求数
    int* planReqUnit;//相应位置的单元是否已经被规划,如果被规划，记录分配的磁盘，否则置-1（防止其它磁盘再读）
    int* planReqTime;//相应位置的单元被规划完成的时间,记录该单元完成读请求的时间，否则置-1（防止其它磁盘再读）
    //每次规划使用
    int* unitReqNumOrder;//哪个单元的req最大，哪个单元就在前面。专门给obj用的buffer。
    
    //价值
    int score = 0;//当前请求剩余的总分数
    int edgeValue = 0;//下一个时间步分数降低多少
    
    std::list<Request*> objRequests;//sorted by time//不包含已完成请求。

    Object(int id, int size, int tag):objId(id),tag(tag),size(size){
        //this->unitReqNum = (int*)malloc((REP_NUM+3)*size*sizeof(int));//malloc分配0空间是合法操作。
        //分配内存
        this->unitReqNum = (int*)malloc(size*(REP_NUM+4)*sizeof(int));
        for(int i=0;i<REP_NUM;i++){
            this->unitOnDisk[i] = this->unitReqNum + size*(i+1);
        }
        this->planReqUnit = this->unitReqNum + size*(REP_NUM+1);
        this->planReqTime = this->unitReqNum + size*(REP_NUM+2);
        this->unitReqNumOrder = this->unitReqNum + size*(REP_NUM+3);//在commitunit时维护。
        //初始化
        for(int i=0;i<size;i++){
            this->unitReqNum[i] = 0;
            this->planReqUnit[i] = -1;
            this->planReqTime[i] = -1;
            this->unitReqNumOrder[i] = i;
        }
    }

    // bool isPlaned(int unitOrder){
    //     if(this->planReqUnit[unitOrder]>=0){
    //         return true;
    //     }
    //     return false;
    // }
    bool isPlaned(int unitOrder){
        if(this->planReqUnit[unitOrder]>=0
            && planReqTime[unitOrder]>=Watch::getTime()){
            return true;
        }
        return false;
    }
    int planedBy(int unitOrder){
        if(this->planReqUnit[unitOrder]>=0){
            return this->planReqUnit[unitOrder];
        }else{
            return -1;
        }
    }
    void plan(int unitOrder, int diskId){
        this->planReqUnit[unitOrder] = diskId;
        this->planReqTime[unitOrder] = Watch::getTime();
    }
    void plan(int unitOrder, int diskId, int timeRequired){
        
        this->planReqUnit[unitOrder] = diskId;
        this->planReqTime[unitOrder] = timeRequired + Watch::getTime();
    }
    void clearPlaned(int unitOrder){
        this->planReqUnit[unitOrder] = -1;
        this->planReqTime[unitOrder] = 0;
    }
    //确认单元之后，需要在reqSpace中删除其它副本。
    

    /// @brief commit a unit which has been read.
    /// @param unitOrder unit order in object
    /// @param doneRequestIds requests completed by this unit will be push_back to this vector.
    void commitUnit(int unitOrder, std::vector<int>* doneRequestIds){
        if(unitOrder>=size){
            throw std::out_of_range("obj unit out of range!");
        }
        
        //删除请求，更新完成请求。
        auto it=objRequests.begin();
        while(it!=objRequests.end()){
            Request* request = *it;
            request->commitUnit(unitOrder);
            if(request->is_done > 0){
                LOG_REQUEST<<"request "<<request->reqId<<" done";
                doneRequestIds->push_back(request->reqId);
                deleteRequest(request->reqId);
                it = objRequests.erase(it);//it会指向被删除元素的下一个元素
            }else{
                it++;
            }
        }
        //把请求该单元的请求数、该单元分配给的磁盘都重置。
        this->unitReqNum[unitOrder] = 0;
        this->clearPlaned(unitOrder);
        //更新请求数顺序
        for(int i=0;i<size;i++){
            if(unitReqNumOrder[i] == unitOrder){
                memmove(unitReqNumOrder+i, unitReqNumOrder+i+1, sizeof(int)*(size-i-1));
                unitReqNumOrder[size-1] = unitOrder;
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
        this->score += START_SCORE*SCORE_FACTOR(size);//同一个对象最多支持2万个请求的计分。超过的话有可能分数值溢出。
        this->edgeValue += PHASE_ONE_EDGE;//开始时每秒减5分，后续阶段由worker更新。

        return newReqUnit;
    }
    //返回取消的请求单元(reqUnit)在obj中的位置(unitOrder)
    std::vector<int> dropRequest(int reqId){
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
        //删除请求
        if((*this->objRequests.begin())->reqId == reqId){
            this->objRequests.erase(this->objRequests.begin());
        }else{
            throw std::logic_error("dropped request shold be the first request!");
        }
        deleteRequest(reqId);
        this->edgeValue -= PHASE_TWO_EDGE;//一个请求已经被删除。更新边缘价值。
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
        int reqUnitSize;
        for(int i=0;i<this->size;i++){
            if(this->unitReqNum[i]>0) {
                reqUnitSize += 1;
            }
        }
        return reqUnitSize;
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


inline void deleteObject(int id){
    
    Object* object = sObjectsPtr[id];
    LOG_OBJECT << "delete object "<< object->objId;
    if(object == &deletedObject){
        LOG_OBJECT<< "object size " << object->size<< "object id "<<object->objId;
        throw std::logic_error("object has been deleted!");
    }
    LOG_OBJECT<< " ";
    sObjectsPtr[id] = &deletedObject;

    delete object;

    LOG_OBJECT<< "deleted over ";
}
inline Object* createObject(int id, int size, int tag){
    LOG_OBJECT << "create object "<<id<<" size "<<size<<" tag "<<tag;
    Object* object = new Object(id, size, tag);
    sObjectsPtr[id] = object;
    return object;
}

LogStream& operator<<(LogStream& s, const Object& obj);
#endif



