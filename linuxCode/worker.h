#ifndef WORKER_H
#define WORKER_H

#include <cstdio>
#include <memory>

#include "bucketData.h"
#include "diskManager.h"
#include "object.h"

//问题：原先是在哪里把object内部list的过期请求erase的？
//在object规划时可能删除，但是由于在overtime中已经删除过一次了，应该会造成重复删除？，
//原先根本没有删除overtime的req（因为overtimeReqTop是0，指向已经被删除的请求。）！！所以就是在object规划时删除的。
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
    
    void freshObjectScore(){
        //auto it = requestedObjects.begin();
        int i = 0;
        while(i != requestedObjects.size()){
            Object* obj = requestedObjects[i];
            //对于overtime，第105个时间片分数还是会减，
            //减完了如果发现分数是0,那么这时候应该只有overtime的req。
            //然后再更新overtime，这时候才会把edgeValue清零。

            //对于phaseTwo，是第11个时间片才开始多减，所以应该在第10个时间片更新。
            obj->clockScore();
            if(obj->score < 0){
                throw std::logic_error("something wrong. score is less than zero. ");
            }
            if(obj->score == 0){
                //此时，对象只有overtime的request。
                if(obj->hasValidRequest() || 
                        obj->edgeValue!=obj->objRequests.size() * PHASE_TWO_EDGE){
                    throw std::logic_error("obj should only has overtime requests,\
                         and number of them equals edgeValue/PHASE_TWO_EDGE");
                }else{
                    std::swap(requestedObjects[i], requestedObjects.back());
                    requestedObjects.pop_back();//换到最后并删除。i不变。
                    // requestedObjects.erase(it);
                }
            }else{
                i++;
            }
        }
    }
    //利用请求的有序性来清除多余请求。但是需要反映到相应对象和存储空间中！
    void clearOvertimeReqAndFreshPhaseTwoReq(){
        assert(overtimeReqTop<=phaseTwoTop);
        int OvertimeTimeBound = Watch::getTime() - EXTRA_TIME;//假设在第0帧创建，request在第105帧后即为0
        //创建时间小于/等于OvertimeTimeBound的都过期了。
        while(requestsPtr[overtimeReqTop] != nullptr &&
            requestsPtr[overtimeReqTop]->createdTime <= OvertimeTimeBound){
            if(requestsPtr[overtimeReqTop] != &deletedRequest){
                auto req = requestsPtr[overtimeReqTop];
                //先消除obj的统计数据影响
                auto obj = sObjectsPtr[req->objId];
                //更新obj内部的Request和单元请求数，更新disk内的请求单元链表。
                auto overtimeReqUnitsOrder = obj->dropRequest(overtimeReqTop);//需要在此之前更新价值
                diskManager.freshOvertimeReqUnits(*obj, overtimeReqUnitsOrder);
            }
            overtimeReqTop += 1;//静态变量加一，循环停止时，当前req指向有效的req或者nullptr。
        }
        //如果对象的score为0,那么更新完overtime之后它的edgeValue也为0


        //在第11帧才转向阶段二并且多减。由于更新阶段时已经更新完分数了。所以在第10个时间片就应该更新阶段
        int phaseTwoTimeBound = Watch::getTime() - PHASE_ONE_TIME;
        //创建时间小于/等于phaseTwoTimeBound的都在下一帧phase2了。
        while(requestsPtr[phaseTwoTop] != nullptr
                && requestsPtr[phaseTwoTop]->createdTime <= phaseTwoTimeBound){
            if(requestsPtr[phaseTwoTop] != &deletedRequest){
                auto req = requestsPtr[phaseTwoTop];
                auto obj = sObjectsPtr[req->objId];
                //补上第二阶段的边缘。这一边缘还未用于更新。是下一个时间步的边缘（这个时间步在规划中使用它，很合理）
                obj->freshPhaseTwoEdge(1-req->is_done);
            }
            phaseTwoTop += 1;//静态变量加一，循环停止时，或者为nullptr，或者上一个req在下一帧正式进入phase2,
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
            std::list<Request*>& requests = obj->objRequests;
            for(auto it=requests.begin();it!=requests.end();it++){//这时候放入的请求应该都没过期且没delete
                requestIds.push_back((*it)->reqId);//获取对象相关的请求id
            }
            for (int i = 0; i < REP_NUM; i++) {
                int diskId = obj->replica[i];
                LOG_BplusTreeN(diskId) << "delete obj " << obj->objId << " reqUnit on deleting it";
            }

            //在被请求对象中删除该对象。
            if(obj->score != 0){
                for(int i=0;i<requestedObjects.size();i++){
                    if(requestedObjects[i] == obj){
                        std::swap(requestedObjects[i], requestedObjects.back());
                        requestedObjects.pop_back();
                        break;
                    }
                    if(i==requestedObjects.size()-1){
                        //未找到对象！！
                        throw std::logic_error("obj has score not zero. \
                            but can't be found in requestedObjects");
                    }
                }
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
        diskManagessssssssr.planObjectsRead();
        
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
