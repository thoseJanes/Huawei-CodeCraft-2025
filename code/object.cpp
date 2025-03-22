#include "object.h"

Object deletedObject(0, -1);//tag是负一。
<<<<<<< HEAD
Request deletedRequest(-1, -1, 0);
=======
Request deletedRequest(-1, -1);
>>>>>>> 7bf56431a960a1eda458cf7ea0726e2f1630f06b
Object* sObjectsPtr[MAX_OBJECT_NUM] = {&deletedObject};
Request* requestPtr[MAX_REQUEST_NUM] = {&deletedRequest};//给第0个赋删除值。
int overtimeReqTop = 0;//在requestPtr中，超时的requestPtr的顶部。
