package com.zd.Util;

public class Request {
    public int objectId;

    public int prevId;

    public boolean isDone;
    public int startTimeStamp;
    public int[] readRequestInfo;
    public boolean isAborted;
    public boolean isGenerated;
    public boolean isBusy;

    public Request(int objectId, int prevId, boolean isDone, int startTimeStamp, int[] readRequestInfo, boolean isAborted, boolean isGenerated) {
        this.objectId = objectId;
        this.prevId = prevId;
        this.isDone = isDone;
        this.startTimeStamp = startTimeStamp;
        this.readRequestInfo = readRequestInfo;
        this.isAborted = isAborted;
        this.isGenerated = isGenerated;
        this.isBusy = false;

    }

    public Request() {
    }
}
