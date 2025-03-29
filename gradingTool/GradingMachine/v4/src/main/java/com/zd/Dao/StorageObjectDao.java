package com.zd.Dao;

import com.zd.Util.StorageObject;

import java.util.List;

public interface StorageObjectDao {
    public void addStorageObject(int objectId, StorageObject storageObject, int writeTimeStamp);
    public void updateStorageObjectDeleteInfo(int objectId, int deleteTimeStamp);
    public StorageObject getStorageObject(int objectId, int timeStamp);
    public List<StorageObject> getStorageObjectsAtTimeStamp(int timeStamp);
    public void clearAll();
}
