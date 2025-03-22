#if !defined(OBJECT_H)
#define OBJECT_H
#include "global.h"
#include "watch.h"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <assert.h>

class Request;
class Object;
extern Object deletedObject;//作为一个被删除/不存在的对象。
extern Request deletedRequest;
extern Object* sObjectsPtr[MAX_OBJECT_NUM];
extern Request* requestsPtr[MAX_REQUEST_NUM];
extern int overtimeReqTop;

class Request {
public:
<<<<<<< HEAD
    Request(int reqId, int objId, int size){
        this->objId = objId;
        this->reqId = reqId;
        this->createdTime = Watch::getTime();
        this->unitFlag = (bool*)malloc(size*sizeof(bool));
        for(int i=0;i<size;i++){
            this->unitFlag[i] = false;
        }
        this->is_done = -size + 1;
=======
    Request(int reqId, int objId){
        this->objId = objId;
        this->reqId = reqId;
        this->createdTime = Watch::getTime();
        this->unitFlag = (bool*)malloc(sObjectsPtr[objId]->size*sizeof(bool));
        memcpy(unitFlag, false, sObjectsPtr[objId]->size*sizeof(bool));
        this->is_done = -sObjectsPtr[objId]->size + 1;
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
    }
    void commitUnit(int unit){
        if(this->unitFlag[unit]){
            is_done += 1;
        }else{
            this->unitFlag[unit] = true;
        }
    }
    int objId;
    int reqId;
    bool* unitFlag;
    int is_done;
    int createdTime;
};
<<<<<<< HEAD

inline void deleteRequest(int id){
    Request* request = requestsPtr[id];
    if(request == &deletedRequest){
        assert(false);
    }
    delete request;
    requestsPtr[id] = &deletedRequest;
}

=======
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
class Object{//由object来负责request的管理（创建/删除）
public:
    Object(int size, int tag){
        this->size = size;
        this->tag = tag;
<<<<<<< HEAD
        this->unitReqNum = (int*)malloc((REP_NUM+3)*size*sizeof(int));//malloc分配0空间是合法操作。
        for(int i=0;i<REP_NUM;i++){
            this->unitOnDisk[i] = this->unitReqNum + (i+1)*size;
        }
        this->planReqUnit = this->unitReqNum + (REP_NUM+1)*size;
        this->planReqTime = this->unitReqNum + (REP_NUM+2)*size;
        for(int i=0;i<size;i++){
            this->planReqUnit[i] = -1;
        }
        for(int i=0;i<size;i++){
            this->planReqUnit[i] = -1;
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
            && planReqTime[unitOrder]==Watch::getTime()){
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
        
        for(int i=objRequests.size()-1;i>=0;i--){
            Request* request = objRequests[i];
            request->commitUnit(unitOrder);
            if(request->is_done){
                doneRequestIds->push_back(request->reqId);
=======
        this->unitReqNum = (int*)malloc((REP_NUM+1)*size*sizeof(int));//malloc分配0空间是合法操作。
        for(int i=0;i<REP_NUM;i++){
            this->unitOnDisk[i] = this->unitReqNum + (i+1)*size;
        }
    }
    std::vector<int> commitUnit(int unit){
        if(unit>=size){
            throw std::out_of_range("obj unit out of range!");
        }
        
        int doneNum;
        std::vector<int> doneRequestIds;//已完成的请求。
        for(int i=objRequests.size()-1;i>=0;i--){
            Request* request = objRequests[i];
            request->commitUnit(unit);
            if(request->is_done){
                doneRequestIds.push_back(request->reqId);
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
                //std::swap(objRequests[i], objRequests.back());swap会改变元素顺序。因此会改变请求到达顺序。
                objRequests.erase(objRequests.begin()+i);
                deleteRequest(request->reqId);
            }
        }
<<<<<<< HEAD
        //把请求该单元的请求数、该单元分配给的磁盘都重置。
        this->unitReqNum[unitOrder] = 0;
        this->clearPlaned(unitOrder);
    }
    Request* createRequest(int reqId){
        Request* request = new Request(reqId, objId, this->size);
=======

        this->unitReqNum[unit] = 0;
        return doneRequestIds;
    }
    Request* createRequest(int reqId){
        Request* request = new Request(reqId, objId);
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
        
        this->objRequests.push_back(request);
        requestsPtr[reqId] = request;
        for(int i=0;i<size;i++){
            this->unitReqNum[i] += 1;
        }

        return request;
    }
<<<<<<< HEAD

    //返回取消的请求单元(reqUnit)在obj中的位置(unitOrder)
=======
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
    std::vector<int> dropRequest(int reqId){
        //更新obj内部的单元请求数，更新disk内的请求单元链表。
        std::vector<int> overtimeReqUnits = {};
        auto req = requestsPtr[reqId];
        for(int i=0;i<this->size;i++){//第几个unit
<<<<<<< HEAD
            if(!req->unitFlag[i]){//如果该请求的该单元还未完成
=======
            if(!req->unitFlag[i]){
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
                this->unitReqNum[i] -= 1;
                if(this->unitReqNum[i]==0){
                    overtimeReqUnits.push_back(i);
                }
            }
        }
        //删除请求对象
        deleteRequest(reqId);
<<<<<<< HEAD
        return overtimeReqUnits;
=======
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
    }
    int size;//大小
    
    //存储时由磁盘设置。
    int replica[REP_NUM];//副本所在磁盘
    int* unitReqNum;//相应位置单元上的剩余请求数
    int* unitOnDisk[REP_NUM];//相应位置的单元存储在磁盘上的位置
<<<<<<< HEAD
    int* planReqUnit;//相应位置的单元是否已经被规划,如果被规划，记录分配的磁盘，否则置-1（防止其它磁盘再读）
    int* planReqTime;

=======
    
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
    int objId;
    int tag;

    std::vector<Request*> objRequests;//sorted by time//不包含已完成请求。
    
    //注意：如果一次取磁盘能满足多个请求，那么可以取得较大效益。
    //可以以Object来组织请求。
    ~Object(){
        for(int i=0;i<3;i++){
            free(this->unitReqNum);//因为分配空间时是整块分配的，所以只需要释放起始空间。
        }

        //删除属于该对象的所有请求。
        for(int i=0;i<objRequests.size();i++){
            deleteRequest(objRequests.at(i)->reqId);//request的生命周期应该由object来掌管吗？
        }
    }
};


<<<<<<< HEAD
=======


>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
inline void deleteObject(int id){
    Object* object = sObjectsPtr[id];
    if(object == &deletedObject){
        assert(false);
    }
    delete object;
    sObjectsPtr[id] = &deletedObject;
}
<<<<<<< HEAD
=======
inline void deleteRequest(int id){
    Request* request = requestsPtr[id];
    if(request == &deletedRequest){
        assert(false);
    }
    delete request;
    requestsPtr[id] = &deletedRequest;
}

>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b


#endif



