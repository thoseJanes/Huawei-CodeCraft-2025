#ifndef WORKER_H
#define WORKER_H

#include <cstdio>
#include <memory>

#include "bucketData.h"
#include "disk.h"
#include "object.h"


class Worker{//obj由它管理。
public:
    Worker(){
        actionBuffer = (char*)malloc((G+10)*sizeof(char));
        memset(actionBuffer, '\0', (G+10)*sizeof(char));
    };
    //初始化磁盘
    void initDisk(){
        for(int i=0;i<N;i++){
            LogFileManager::flushAll();
            diskManager.addDisk(V);
        }
    }
    //接收头部统计数据
    void swallowStatistics(){
        int StaNum = (T - 1) / FRE_PER_SLICING + 1;
        int* start = (int*)malloc(3*M*StaNum*sizeof(int));
        int** ptrStart = (int**)malloc(3*M*sizeof(int*));
        int* temp = start;
        for(int k=0;k<3;k++){
            for(int i=0;i<M;i++){
                ptrStart[i+M*k] = temp;
                temp = temp + StaNum;
            }
        }
        StatisticsBucket::initBuckData(ptrStart, ptrStart+M, ptrStart+2*M);

        for (int k=1; k <= 3; k++){
            for (int i = 1; i <= M; i++) {
                for (int j = 1; j <= StaNum; j++) {
                    scanf("%d", start);//start已经是指针位置了。
                    start += 1;
                }
            }
        }
    }
    //利用请求的有序性来清除多余请求。但是需要反映到相应对象和存储空间中！
    void clearOvertimeReq(){
        int timeBound = Watch::getTime() - 105;
        //删除的request的reqId为-1.
        while(requestsPtr[overtimeReqTop]->reqId>=0 && timeBound > requestsPtr[overtimeReqTop]->createdTime){
            auto req = requestsPtr[overtimeReqTop];
            //先消除obj的统计数据影响
            auto obj = sObjectsPtr[req->objId];
            //删除obj内部的请求。这是为了确认顺序。主要用于log阶段。其实可以删除。
            {
                if(obj->objRequests[0]==req){
                    ;
                }else{
                    assert(false);
                }
            }
            //更新obj内部的Request和单元请求数，更新disk内的请求单元链表。
            auto overtimeReqUnitsOrder = obj->dropRequest(overtimeReqTop);
            diskManager.freshOvertimeReqUnits(*obj, overtimeReqUnitsOrder);

            overtimeReqTop += 1;//静态变量加一，循环停止时，当前req即有效。
        }
    }
    //刷新磁盘磁头令牌
    void freshDiskTokens(){
        this->diskManager.freshTokens();
    }
    //同步时间
    void correctWatch(){
        int timestamp;
        scanf("%*s%d", &timestamp);
        LOG_IPCINFO << "[interactor]\n" << "TIMESTAMP " << timestamp;
        Watch::correct(timestamp);
        printf("TIMESTAMP %d\n", Watch::getTime());
        fflush(stdout);
        LOG_IPCINFO << "[player]\n" << "TIMESTAMP "<< Watch::getTime();
    };
    
    //接收、处理、输出删除数据。
    void processDelete(){
        LOG_DISK << "TimeStamp" << Watch::getTime();
        LOG_DISK << "process delete";
        //接收数据和处理数据
        int n_delete;
        int id;
        std::vector<int> requestIds = {};
        scanf("%d", &n_delete);
        LOG_IPCINFO << "[interactor]\n" << n_delete;
        for (int o = 0; o < n_delete; o++) {
            scanf("%d", &id);
            LOG_IPCINFO  << id;
            Object* obj = sObjectsPtr[id];//获取要删除的对象
            HistoryBucket::addDel(1, obj->tag);//更新bucket数据
            std::vector<Request*> requests = obj->objRequests;
            for(int i=0;i<requests.size();i++){
                requestIds.push_back(requests[i]->reqId);//获取对象相关的请求id
            }
            for (int i = 0; i < REP_NUM; i++) {
                int diskId = obj->replica[i];
                LOG_BplusTreeN(diskId) << "delete obj " << obj->objId << " reqUnit on deleting it";
            }
            
            LOG_OBJECT << "start free space";
            diskManager.freeSpace(*obj);//删除磁盘空间。以及清除相应请求。
            LOG_OBJECT << "start delete object";
            deleteObject(id);//会删除obj和其关联的request
        }
        
        //输出数据
        printf("%d\n", static_cast<int>(requestIds.size()));
        LOG_IPCINFO << "[player]\n" << requestIds.size();
        for(int i=0;i<requestIds.size();i++){
            printf("%d\n", requestIds[i]);
            LOG_IPCINFO << requestIds[i];
        }
        fflush(stdout);
    }
    //接收、处理、输出写入数据。
    void processWrite(){
        LOG_DISK << "process write";
        int n_write;
        scanf("%d", &n_write);
        LOG_IPCINFO << "[interactor]\n" << n_write;
        for (int i = 0; i < n_write; i++) {
            int id, size, tag;
            scanf("%d%d%d", &id, &size, &tag);
            LOG_IPCINFO << id<<" "<<size<< " "<<tag;
            tag -= 1;//内部用0 - M-1表示tag。输入、输出时都需要注意。
            LOG_DISK << "get wrt" <<"id:"<<id<<", size:"<<size<<", tag:"<<tag;
            HistoryBucket::addWrt(1, tag);
            LOG_DISK << "add wrt data to historyBucket over";
            Object* obj = createObject(id, size, tag);
            LOG_DISK << "create object over";
            diskManager.assignSpace(*obj);
            LOG_DISK << "assignSpace over" << n_write;
            LOG_DISK << "object" << *obj;

            printf("%d\n", id);
            LOG_IPCINFO << "[player]\n" << id ;
            for (int j = 0; j < REP_NUM; j++) {
                printf("%d", obj->replica[j]+1);//内部用0 - N-1表示磁盘
                for (int k = 0; k < size; k++) {
                    printf(" %d", obj->unitOnDisk[j][k]+1);//内部用0 - V表示磁盘单元
                }
                printf("\n");
                LOG_IPCINFO << "disk " << obj->replica[j] + 1 <<"space...";
            }
        }
        fflush(stdout);
    }

    void actionsToChars(char* actionBuffer, std::vector<HeadOperator> headOperation){
        int bufCur = 0;
        if(headOperation.size()>0 && headOperation[0].action==JUMP){//如果是跳操作。
            bufCur += snprintf(actionBuffer, G+10, "j %d", headOperation[0].jumpTo+1);//注意内部表示与外部值的转换。+1
        }else{//如果是读或走操作
            for(int j=0;j<headOperation.size();j++){//多个操作遍历
                auto operate = headOperation[j];
                if(operate.action == PASS){
                    for(int k=0;k<operate.times;k++){
                        actionBuffer[bufCur] = 'p';
                        bufCur++;
                    }
                }else if(operate.action == READ){
                    for(int k=0;k<operate.times;k++){
                        actionBuffer[bufCur] = 'r';
                        bufCur++;
                    }
                }
            }
            actionBuffer[bufCur] = '#';bufCur++;
            actionBuffer[bufCur] = '\0';//在末尾添加字符。
        }
        LOG_ACTIONS << "actionBuffer:" << actionBuffer << ", headOperation" << headOperation;
    }

    void processRead(){
        //接收数据并创建对象
        int n_read;
        int request_id, object_id;
        scanf("%d", &n_read);
        LOG_IPCINFO << "[interactor]\n" << n_read ;
        for (int i = 1; i <= n_read; i++) {
            scanf("%d%d", &request_id, &object_id);
            LOG_IPCINFO << request_id<< " " << object_id ;
            Object* obj = sObjectsPtr[object_id];
            HistoryBucket::addReq(1, obj->tag);

            auto newReqUnits = obj->createRequest(request_id);
            diskManager.freshNewReqUnits(*obj, newReqUnits);
        }
        
        //规划读取过程。
        diskManager.planUnitsRead();
        
        //输出读取过程。
        for(int i=0;i<N;i++){//不同磁盘
            auto headOperation = diskManager.getHandledOperations(i);
            LOG_ACTIONSN(i)<< "\n\ntobeComplete:" << diskManager.diskGroup[i]->disk->head.toBeComplete;
            LOG_ACTIONSN(i)<< "presentTokens:" << diskManager.diskGroup[i]->disk->head.presentTokens;
            LOG_ACTIONSN(i)<< "nextReadConsume:" << diskManager.diskGroup[i]->disk->head.readConsume;
            actionsToChars(actionBuffer, headOperation);
            printf("%s\n", actionBuffer);
            LOG_IPCINFO << "[player]\n" << actionBuffer ;
        }

        //输出完成的请求。
        auto doneReqIds = diskManager.getDoneRequests();
        printf("%d\n", static_cast<int>(doneReqIds.size()));
        LOG_IPCINFO << doneReqIds.size();
        for(int i=0;i<doneReqIds.size();i++){
            printf("%d\n", doneReqIds[i]);
            LOG_IPCINFO << doneReqIds[i];
        }
    
        fflush(stdout);
    }

    void checkDisk(Disk disk);//更新disk的信息
    ~Worker(){
        free(actionBuffer);
    }
private:
    DiskManager diskManager;
    char* actionBuffer;
};


#endif
