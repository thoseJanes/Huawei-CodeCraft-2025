#ifndef WORKER_H
#define WORKER_H

#include <cstdio>
#include <memory>

#include "bucketData.h"
#include "disk.h"
#include "object.h"


class Worker{//obj由它管理。
public:
<<<<<<< HEAD
    Worker(){
        actionBuffer = (char*)malloc((G+10)*sizeof(char));
        memset(actionBuffer, '\0', (G+10)*sizeof(char));
    };
    //初始化磁盘
    void initDisk(){
        for(int i=0;i<N;i++){
            diskManager.addDisk(V);
        }
=======
    Worker(){actionBuffer = (char*)malloc((G+10)*sizeof(char));memcpy(actionBuffer, '\0', (G+10)*sizeof(char));};
    //初始化磁盘
    void initDisk(){
        diskManager.addDisk(V);
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
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
<<<<<<< HEAD
            auto overtimeReqUnitsOrder = obj->dropRequest(overtimeReqTop);
            diskManager.freshOvertimeReqUnits(*obj, overtimeReqUnitsOrder);
=======
            auto overtimeReqUnits = obj->dropRequest(overtimeReqTop);
            diskManager.freshOvertimeReqUnits(*obj, overtimeReqUnits);
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b

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
        int timeDelay = Watch::correct(timestamp);
        printf("TIMESTAMP %d\n", Watch::getTime());
        fflush(stdout);
    };
    
    
    //接收、处理、输出删除数据。
    void processDelete(){
        //接收数据和处理数据
        int n_delete;
        int id;
        std::vector<int> requestIds = {};
        scanf("%d", &n_delete);
        for (int i = 1; i <= n_delete; i++) {
            scanf("%d", &id);
            Object* obj = sObjectsPtr[id];
            HistoryBucket::addDel(1, obj->tag);
            std::vector<Request*> requests = obj->objRequests;
            for(int i=0;i<requests.size();i++){
                requestIds.push_back(requests[i]->reqId);
            }
            diskManager.freeSpace(*obj);//删除磁盘空间。
            deleteObject(id);//会删除obj和其关联的request
        }

        //输出数据
        printf("%d\n", requestIds.size());
        for(int i=0;i<requestIds.size();i++){
            printf("%d\n", requestIds[i]);
        }
        fflush(stdout);
    }
    //接收、处理、输出写入数据。
    void processWrite(){
        int n_write;
        scanf("%d", &n_write);
        for (int i = 0; i < n_write; i++) {
            int id, size, tag;
            scanf("%d%d%d", &id, &size, &tag);
            tag -= 1;//内部用0 - M-1表示tag。输入、输出时都需要注意。
            HistoryBucket::addWrt(1, tag);
            Object* obj = new Object(id, tag);
            diskManager.assignSpace(*obj);
    
            printf("%d\n", id);
            for (int j = 0; j < REP_NUM; j++) {
                printf("%d", obj->replica[j]+1);//内部用0 - N-1表示磁盘
                for (int k = 0; k < size; k++) {
                    printf(" %d", obj->unitOnDisk[j][k]+1);//内部用0 - V表示磁盘单元
                }
                printf("\n");
            }
        }
        fflush(stdout);
    }

<<<<<<< HEAD
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
                    }
                    bufCur += operate.times;
                }else if(operate.action == READ){
                    for(int k=0;k<operate.times;k++){
                        actionBuffer[bufCur] = 'r';
                    }
                }
            }
            actionBuffer[bufCur++] = '#';
            actionBuffer[bufCur] = '\0';//在末尾添加字符。
        }
    }

=======
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
    void processRead(){
        //接收数据并创建对象
        int n_read;
        int request_id, object_id;
        scanf("%d", &n_read);
        for (int i = 1; i <= n_read; i++) {
            scanf("%d%d", &request_id, &object_id);
            Object* obj = sObjectsPtr[object_id];
            HistoryBucket::addReq(1, obj->tag);

            obj->createRequest(request_id);
            diskManager.freshNewReqUnits(*obj);
        }
        
        //规划读取过程。
<<<<<<< HEAD
        diskManager.planUnitsRead();
        
        //输出读取过程。
        for(int i=0;i<N;i++){//不同磁盘
            auto headOperation = diskManager.getHandledOperations(i);
            actionsToChars(actionBuffer, headOperation);
            printf("%s\n", actionBuffer);
        }

        //输出完成的请求。
        auto doneReqIds = diskManager.getDoneRequests();
        printf("%d\n", doneReqIds.size());
        for(int i=0;i<doneReqIds.size();i++){
            printf("%d\n", doneReqIds[i]);
=======
        std::vector<int> doneReqId = {};
        auto headOperations = diskManager.planHeadMove(doneReqId);
        
        //输出读取过程。
        for(int i=0;i<N;i++){//不同磁盘
            auto headOperation = headOperations[i];
            int bufCur = 0;
            if(headOperation.size()>0 && headOperation[0].first==JUMP){//如果是跳操作。
                bufCur += snprintf(actionBuffer, G+10, "j %d", headOperation[0].second+1);//注意内部表示与外部值的转换。+1
            }else{//如果是读或走操作
                for(int j=0;j<headOperation.size();j++){//多个操作遍历
                    auto operate = headOperation[j];
                    if(operate.first == PASS){
                        for(int k=0;k<operate.second;k++){
                            actionBuffer[bufCur] = 'p';
                        }
                        bufCur += operate.second;
                    }else if(operate.first == READ){
                        for(int k=0;k<operate.second;k++){
                            actionBuffer[bufCur] = 'r';
                        }
                    }
                }
                actionBuffer[bufCur++] = '#';
                actionBuffer[bufCur] = '\0';//在末尾添加字符。
            }
            printf("%s\n", actionBuffer);
        }
        //输出完成的请求。
        printf("%d\n", doneReqId.size());
        for(int i=0;i<doneReqId.size();i++){
            printf("%d\n", doneReqId[i]);
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
        }
    
        fflush(stdout);
    }

<<<<<<< HEAD
    void checkDisk(Disk disk);//更新disk的信息
    ~Worker(){
        free(actionBuffer);
    }
=======



    void checkDisk(Disk disk);//更新disk的信息
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
private:
    DiskManager diskManager;
    char* actionBuffer;
    

};


#endif