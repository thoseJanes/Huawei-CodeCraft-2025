#include "object.h"

Object deletedObject(0, 0, -1);//id为0 tag是负一。
Request deletedRequest(-1, -1, 0);
Object* sObjectsPtr[MAX_OBJECT_NUM] = {&deletedObject};
Request* requestsPtr[MAX_REQUEST_NUM] = {&deletedRequest};//给第0个赋删除值。
int overtimeReqTop = 0;//在requestPtr中，超时的requestPtr的顶部。

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
        s << obj.planReqUnit[j] << ", ";
    }
    s << ", plan time:";
    for (int j = 0; j < obj.size; j++) {
        s << obj.planReqTime[j] << ", ";
    }
    
    s<<"}\n";
    return s;
}
