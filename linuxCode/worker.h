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
    Worker(int tagNum):diskManager(tagNum){
        actionBuffer = (char*)malloc((G+10)*sizeof(char));
        memset(actionBuffer, '\0', (G+10)*sizeof(char));
    };
    //初始化磁盘
    void addDisk(std::vector<std::pair<int, int>>  tagToSpaceSize){
        LogFileManager::flushAll();
        diskManager.addDisk(V, tagToSpaceSize);
    }
    //接收头部统计数据
    void swallowStatistics(){
        int StaNum = (T - 1) / FRE_PER_SLICING + 1;
        int* start = (int*)malloc(3*M*StaNum*sizeof(int));
        memset(start, 0, 3 * M * StaNum * sizeof(int));
        int** ptrStart = (int**)malloc(3*M*sizeof(int*));
        int* temp = start;
        for(int k=0;k<3;k++){
            for(int i=0;i<M;i++){
                ptrStart[i+M*k] = temp;
                temp = temp + StaNum;
            }
        }
        //[M][StaNum]
        StatisticsBucket::initBuckData(ptrStart, ptrStart+M, ptrStart+2*M);
        for (int k=1; k <= 3; k++){
            for (int i = 1; i <= M; i++) {
                for (int j = 1; j <= StaNum; j++) {
                    scanf("%d", start);//start已经是指针位置了。
                    start += 1;
                }
            }
        }
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < StaNum; j++) {
                LOG_IPCINFO << StatisticsBucket::reqSta[i][j];
            }
        }

        int* tagObjects = new int[M+1];
        int* tagDiskSize = new int[M];
        int* tagLeftSize = new int[M];
        tagObjects[M] = 0;
        for(int i =0;i<M;i++){
            tagObjects[i] = 0;
            for (int j = 0; j < StaNum; j++) {
                tagObjects[i] += StatisticsBucket::wrtSta[i][j] - StatisticsBucket::delSta[i][j];
            }
            tagObjects[M] += tagObjects[i];//总的对象数目。
        }

        //计算相关性
        double* relativity = (double*)malloc(sizeof(double)*M*M);
        for (int i = 0; i < M; i++) {
            relativity[i * M + i] = 0;//请求总数。
            for (int j = 0; j < StaNum; j++) {
                relativity[i * M + i] += StatisticsBucket::reqSta[i][j];
            }
        }
        for (int i1 = 0; i1 < M; i1++) {
            for (int i2 = 0; i2 < i1; i2++) {
                relativity[i1 * M + i2] = 0;
                for (int j = 0; j < StaNum; j++) {
                    relativity[i1 * M + i2] += 1.0* StatisticsBucket::reqSta[i1][j] * StatisticsBucket::reqSta[i2][j];
                }
                relativity[i1 * M + i2] /= (1.0 * relativity[i1 * M + i1] * relativity[i2 * M + i2]);
                relativity[i2 * M + i1] = relativity[i1 * M + i2];
            }
        }

        StatisticsBucket::initRelativity(relativity);
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < M; j++) {
                LOG_IPCINFO << relativity[i*M+j];
            }
        }

        //确定每个tag将要分配的磁盘空间大小。
        int tagNumPerDisk = ((M * 3 / N) / 2 + 1) * 2;//每个磁盘大概分配多少个tag
        int unitLeft = 0;
        for (int i = 0; i < M; i++) {
            tagDiskSize[i] = N * V * tagObjects[i] / tagObjects[M];//某个tag在某个磁盘上应该分配多少空间。
            tagLeftSize[i] = tagDiskSize[i];
            unitLeft += tagDiskSize[i];
        }
        unitLeft = N * V - unitLeft; assert(unitLeft >= 0);
        tagDiskSize[M - 1] += unitLeft;//所有的tagDiskSize加起来正好为所有单元大小。
        tagLeftSize[M - 1] += tagDiskSize[M - 1];

        //将tag两两分为一组。按每个tag的对象比例分配空间。如果tag数量为奇数呢？那就先找一个和其它tag相关性最高的tag。
        std::vector<int> tagIn = {};
        int tagStart = 0;
        tagIn.push_back(tagStart);
        for (int i = 0; i < M/2; i++) {
            tagIn.push_back(StatisticsBucket::getMaxRltTag(tagIn.back(), tagIn));
            for (int j = 0; j < M; j++) {
                if (std::find(tagIn.begin(), tagIn.end(), j) == tagIn.end()) {
                    tagIn.push_back(j);
                    break;
                }
            }
        }
        tagIn.erase(tagIn.begin()); tagIn.push_back(0);//现在，第2、3个tag，4、5个tag...都有较高相关性。
        //确定每种tag给每个磁盘的数量。
        std::vector<std::vector<std::pair<int, int>>> diskTagSize;
        for (int i = 0; i < N; i++) {
            diskTagSize.push_back({});
            int left = V;
            for (int j = 0; j < M-1; j++) {
                int tag = tagIn[j];
                int tagSize = static_cast<int>(1.0 * V * tagObjects[tag] / tagObjects[M]);
                left -= tagSize;
                diskTagSize.back().push_back({ tag, tagSize });
            }
            diskTagSize.back().push_back({ tagIn[M - 1], left });
            addDisk(diskTagSize.back());
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
                //这里的原因是，对象处理完了所有请求，所以分数为0了，但是刚好又有超时请求，所以边缘不为0？
                LOG_OBJECT << *obj;
                assert(!obj->hasValidRequest());
                assert(obj->edgeValue == obj->objRequests.size() * PHASE_TWO_EDGE * SCORE_FACTOR(obj->size));
                assert(obj->score == -obj->edgeValue);
                obj->score = 0;//把负的分数恢复为0.edgeValue在overtime中处理。
                
                //throw std::logic_error("something wrong. score is less than zero. ");
            }
            if(obj->score == 0){
                //此时，对象只有overtime的request。
                if(obj->hasValidRequest() || 
                        obj->edgeValue!=obj->objRequests.size() * PHASE_TWO_EDGE * SCORE_FACTOR(obj->size)){
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
            if(requestsPtr[overtimeReqTop] != deletedRequest){
                auto req = requestsPtr[overtimeReqTop];
                //先消除obj的统计数据影响
                auto obj = sObjectsPtr[req->objId];
                //更新obj内部的Request和单元请求数，更新disk内的请求单元链表。
                auto overtimeReqUnitsOrder = obj->dropOvertimeRequest(overtimeReqTop);//需要在此之前更新价值
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
            if(requestsPtr[phaseTwoTop] != deletedRequest){
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
        std::vector<Object*> objs = {};
        scanf("%d", &n_delete);
        LOG_IPCINFO << "[interactor]\n" << n_delete;
        int tolDeleteReqNum = 0;
        for (int o = 0; o < n_delete; o++) {
            scanf("%d", &id);
            Object* obj = sObjectsPtr[id];//获取要删除的对象
            objs.push_back(obj);
            tolDeleteReqNum += obj->objRequests.size();
            tolDeleteReqNum += obj->overtimeRequests.size();
            LOG_IPCINFO << id;
        }
        printf("%d\n", tolDeleteReqNum);
        LOG_IPCINFO << "[player]\n" << tolDeleteReqNum;

        for (int o = 0; o < n_delete; o++) {
            Object* obj = objs[o];
            HistoryBucket::addDel(1, obj->tag);//更新bucket数据
            std::list<Request*>& requests = obj->objRequests;
            for (auto it = obj->overtimeRequests.begin(); it != obj->overtimeRequests.end(); it++) {
                printf("%d\n", *it);
                LOG_IPCINFO << *it;
            }
            for(auto it=requests.begin();it!=requests.end();it++){//这时候放入的请求应该都没过期且没delete
                printf("%d\n", (*it)->reqId);
                LOG_IPCINFO << (*it)->reqId;
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
            deleteObject(obj->objId);//会删除obj和其关联的request
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
                    for(int k=0;k<operate.passTimes;k++){
                        actionBuffer[bufCur] = 'p';
                        bufCur++;
                    }
                }else if(operate.action == READ || operate.action == VREAD){
                    actionBuffer[bufCur] = 'r';
                    bufCur++;
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
            if (obj->score == 0) {
                requestedObjects.push_back(obj);
            }
            auto newReqUnits = obj->createRequest(request_id);
            diskManager.freshNewReqUnits(*obj, newReqUnits);
        }
        
        //规划读取过程。
        //前期规划————（受planed制约）
        //后期优化算子————
        //跳移拼接算子：把跳的action处分开，然后拼接。
        //对象变副本算子：把对象副本放到规划少的磁头去读
        //单元变副本算子：把单元副本放到规划少的磁头去读
        diskManager.testMultiReadStrategy();
        
        std::vector<int> diskHeadPos;
        //输出读取过程。
        for(int i=0;i<N;i++){//不同磁盘
            auto headOperation = diskManager.getHandledOperations(i);
            LOG_ACTIONSN(i)<< "\n\ntobeComplete:" << diskManager.diskGroup[i]->disk->head.toBeComplete;
            LOG_ACTIONSN(i)<< "presentTokens:" << diskManager.diskGroup[i]->disk->head.presentTokens;
            LOG_ACTIONSN(i)<< "nextReadConsume:" << diskManager.diskGroup[i]->disk->head.readConsume;
            actionsToChars(actionBuffer, headOperation);
            printf("%s\n", actionBuffer);
            LOG_IPCINFO << "[player]\n" << actionBuffer ;
            diskHeadPos.push_back(diskManager.getPlanner(i)->getDisk()->head.headPos);
        }

        LOG_IPCINFO << "Disk head position:" << diskHeadPos;

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
    const DiskManager* getDiskManager(){
        return &diskManager;
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
