package com.zd.Util;

public class DiskOperation {
    public int objectId;
    public String operationType;
    public int timeStamp;


    public DiskOperation(int objectId, String operationType, int timeStamp) {
        this.objectId = objectId;
        this.operationType = operationType;
        this.timeStamp = timeStamp;
    }
}
