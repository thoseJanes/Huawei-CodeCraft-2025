package com.zd.Util;

public class StorageObjectionWriteInfo {
    public int objectId;
    public StorageObject storageObjectl;
    public int writeTimeStamp;

    public StorageObjectionWriteInfo() {
    }

    public StorageObjectionWriteInfo(int objectId, StorageObject storageObjectl, int writeTimeStamp) {
        this.objectId = objectId;
        this.storageObjectl = storageObjectl;
        this.writeTimeStamp = writeTimeStamp;
    }
}
