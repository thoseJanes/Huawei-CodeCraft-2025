package com.zd.Dao;

import com.zd.Util.DiskOperation;

import java.util.List;

public interface DiskDao {
    public void addDiskDeleteInfo(int timeStamp, int objectId);
    public void addDiskWriteInfo(int timeStamp, int objectId);
    public void clearAll();
    public List<DiskOperation> getDiskOperation(int startTimeStamp, int endTimeStamp);
}
