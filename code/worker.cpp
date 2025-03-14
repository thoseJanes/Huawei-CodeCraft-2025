#include "worker.h"

StorageObject deletedObject(0);
Request doneRequest(-1, -1);
StorageObject* sObjectsPtr[MAX_OBJECT_NUM];
Request* requestPtr[MAX_REQUEST_NUM];