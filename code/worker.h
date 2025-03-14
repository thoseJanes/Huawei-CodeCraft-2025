#if !defined(WORKER_H)
#define WORKER_H
#include "global.h"
#include <algorithm>

#define Object StorageObject
class Request;
class StorageObject;
extern StorageObject* sObjectsPtr[MAX_OBJECT_NUM];
extern Request* requestsPtr[MAX_REQUEST_NUM];
extern StorageObject deletedObject;//作为一个被删除/不存在的对象。
extern Request deletedRequest;

class StorageObject{//由object来负责request的管理（创建/删除）
public:
    StorageObject(int size){
        this->size = size;
        this->unitReqNum = (int*)malloc((REP_NUM+1)*size*sizeof(int));//malloc分配0空间是合法操作。
        for(int i=0;i<3;i++){
            this->unitOnDisk[i] = this->unitReqNum + (i+1)*size;
        }
    }
    void commitUnit(int unit){
        if(unit>=size){
            throw std::out_of_range("obj unit out of range!");
        }
        
        int doneNum;
        for(int i=objRequests.size()-1;i>=0;i--){
            Request* request = objRequests[i];
            request->commitUnit(unit);
            if(request->is_done){
                doneRequestIds.push_back(request->reqId);
                //std::swap(objRequests[i], objRequests.back());swap会改变元素顺序。因此会改变请求到达顺序。
                objRequests.erase(objRequests.begin()+i);
                deleteRequest(request->reqId);
            }
        }

        this->unitReqNum[unit] = 0;
    }
    Request* createRequest(int reqId){
        Request* request = new Request(reqId, objId);
        if(request->objId == this->objId){
            this->objRequests.push_back(request);
            for(int i=0;i<size;i++){
                this->unitReqNum[i] += 1;
            }
        }else{
            assert((request->objId == this->objId));
        }
        return request;
    }
    int size;//大小
    
    //存储时由磁盘设置。
    int replica[REP_NUM];//副本所在磁盘
    int* unitReqNum;//相应位置单元上的剩余请求数
    int* unitOnDisk[REP_NUM];//相应位置的单元存储在磁盘上的位置
    
    int objId;

    std::vector<Request*> objRequests;//sorted by time//不包含已完成请求。
    std::vector<int> doneRequestIds;//已完成的请求。
    //注意：如果一次取磁盘能满足多个请求，那么可以取得较大效益。
    //可以以Object来组织请求。
    ~StorageObject(){
        for(int i=0;i<3;i++){
            free(this->unitReqNum);//因为分配空间时是整块分配的，所以只需要释放起始空间。
        }

        //删除属于该对象的所有请求。
        for(int i=0;i<objRequests.size();i++){
            deleteRequest(objRequests.at(i)->reqId);//request的生命周期应该由object来掌管吗？
        }
    }
};
class Request {
public:
    Request(int reqId, int objId){
        this->objId = objId;
        this->reqId = reqId;
        this->unitFlag = (bool*)malloc(sObjectsPtr[objId]->size*sizeof(bool));
        this->is_done = -sObjectsPtr[objId]->size+1;
    }
    void commitUnit(int unit){
        if(this->unitFlag[unit]){
            is_done += 1;
        }else{
            this->unitFlag[unit] = 1;
        }
    }
    int objId;
    int reqId;
    bool* unitFlag;
    int is_done;
};


inline void deleteObject(int id){
    StorageObject* object = sObjectsPtr[id];
    if(object == &deletedObject){
        assert(!(object == &deletedObject));
    }
    delete object;
    sObjectsPtr[id] = &deletedObject;
}
inline void deleteRequest(int id){
    Request* request = requestsPtr[id];
    if(request == &deletedRequest){
        assert(!(request == &deletedRequest));
    }
    delete request;
    requestsPtr[id] = &deletedRequest;
}


#endif // OPERATION_H



