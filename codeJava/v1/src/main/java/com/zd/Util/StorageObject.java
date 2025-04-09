package com.zd.Util;

import java.util.Arrays;

public class StorageObject {
    public static int REP_NUM = 3;
    public int[] replica = new int[REP_NUM];

    public int[][] unit = new int[REP_NUM][];

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

        for(int i=0;i<REP_NUM;i++){
            unit[i] = new int[size];
            Arrays.fill(unit[i],-1);
        }
    }

    public StorageObject() {
    }
}
