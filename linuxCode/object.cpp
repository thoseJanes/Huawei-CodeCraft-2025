#include "object.h"
#include "LogTool.h"

Object* deletedObject = new Object(0, 0, -1);//id为0 tag是负一。
Request* deletedRequest = new Request(-1, -1, 0);
Object* sObjectsPtr[MAX_OBJECT_NUM] = {deletedObject};//这样其他值会被赋值为nullptr
Request* requestsPtr[MAX_REQUEST_NUM] = {deletedRequest};//给第0个赋删除值。
#ifdef ENABLE_OBJECTSCORE
std::vector<Object*> requestedObjects = {};//存在请求需要计算分数的对象。
#endif
int overtimeReqTop = 0;//在requestPtr中，超时的request的顶部,也即指向第一个未超时的request。
int phaseTwoTop = 0;//在requestPtr中，进入第二阶段的request的顶部,也即指向第一个未进入第二阶段的request。


LogStream& operator<<(LogStream& s, const Object& obj){
    s << "{objId:" << obj.objId << ", tag:" << obj.tag << ", size:" << obj.size << "\n";
    for(int i=0;i<REP_NUM;i++){
        s << "(replica:" << obj.replica[i] << ", address:";
        for(int j=0;j<obj.size;j++){
            s << obj.unitOnDisk[i][j] << ", ";
        }
            
        s << ")\n";
    }
    s << ", req num:";
    for (int j = 0; j < obj.size; j++) {
        s << obj.unitReqNum[j] << ",";
    }
    s << ", plan for disk:";
    for (int j = 0; j < obj.size; j++) {
        s << obj.planReqDisk[j] << ", ";
    }
    s << ", plan time:";
    for (int j = 0; j < obj.size; j++) {
        s << obj.planReqTime[j] << ", ";
    }
    s << ", virPlan:";
    for (int j = 0; j < obj.size; j++) {
        s << obj.planBuffer[j] << ", ";
    }
    s << ", coscore:";
    for (int j = 0; j < obj.size; j++) {
        s << obj.coScore[j] << ", ";
    }
    s << ", coedge:";
    for (int j = 0; j < obj.size; j++) {
        s << obj.coEdgeValue[j] << ", ";
    }
    
    s<<"}\n";
    return s;
}
