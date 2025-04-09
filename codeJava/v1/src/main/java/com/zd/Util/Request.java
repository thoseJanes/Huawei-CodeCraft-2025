package com.zd.Util;

public class Request {
    public int objectIndex;

    public int prevIndex;

    public boolean isDone;
    public int startTimeStamp;
    public int[] readRequestInfo;
    public boolean isAborted;
    public boolean isGenerated;
    public boolean isBusy;

    public Request(int objectIndex, int prevIndex, boolean isDone, int startTimeStamp, int[] readRequestInfo, boolean isAborted, boolean isGenerated) {
        this.objectIndex = objectIndex;
        this.prevIndex = prevIndex;
        this.isDone = isDone;
        this.startTimeStamp = startTimeStamp;
        this.readRequestInfo = readRequestInfo;
        this.isAborted = isAborted;
        this.isGenerated = isGenerated;

    }

    public Request(int objectIndex, int objectSize, int prevIndex, int startTimeStamp) {
        this.objectIndex = objectIndex;
        this.prevIndex = prevIndex;
        this.isDone = false;
        this.startTimeStamp = startTimeStamp;
        this.readRequestInfo = new int[objectSize];
        this.isAborted = false;
        this.isGenerated = true;
        this.isBusy = false;
    }
}
