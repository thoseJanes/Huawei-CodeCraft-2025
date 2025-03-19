package com.zd.Util;

public class Object {
    public static int REP_NUM;
    public int[] replica = new int[REP_NUM+1];

    public int[][] unit = new int[REP_NUM+1][];

    public int size;

    public int lastRequestPoint;

    public boolean isDelete;
    public int tag;
}
