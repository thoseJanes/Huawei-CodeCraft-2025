package com.zd.Util;

public class StorageObject {
    public static int REP_NUM;
    public int[] replica = new int[REP_NUM+1];

    public int[][] unit = new int[REP_NUM+1][];

    public int size;

    public int lastRequestPoint;

    public boolean isDelete;
    public int tag;
    public int requestsNum;
    public double scores;
    public int scoresLevel;
    public double desScore;

    public StorageObject(int size, int lastRequestPoint, boolean isDelete, int tag, int requestsNum, double scores, int scoresLevel) {
        this.size = size;
        this.lastRequestPoint = lastRequestPoint;
        this.isDelete = isDelete;
        this.tag = tag;
        this.requestsNum = requestsNum;
        this.scores = scores;
        this.scoresLevel = scoresLevel;
        this.desScore = 0;
    }

    public StorageObject() {
    }
}
