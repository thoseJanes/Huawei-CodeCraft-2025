package com.zd.Util;

public class StorageObjectionDeleteInfo {
    public int objectId;
    public int deleteTimeStamp;

    public StorageObjectionDeleteInfo() {
    }

    public StorageObjectionDeleteInfo(int objectId, int deleteTimeStamp) {
        this.objectId = objectId;
        this.deleteTimeStamp = deleteTimeStamp;
    }
}
