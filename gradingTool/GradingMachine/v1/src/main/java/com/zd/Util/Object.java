package com.zd.Util;

public class Object {
    static int REP_NUM;
    int[] replica = new int[REP_NUM + 1];

    int[][] unit = new int[REP_NUM + 1][];

    int size;

    int lastRequestPoint;

    boolean isDelete;
}
