#include "object.h"

Object deletedObject(0, -1);//tag是负一。
Request deletedRequest(-1, -1);
Object* sObjectsPtr[MAX_OBJECT_NUM] = {&deletedObject};
Request* requestPtr[MAX_REQUEST_NUM] = {&deletedRequest};//给第0个赋删除值。
int overtimeReqTop = 0;//在requestPtr中，超时的requestPtr的顶部。
