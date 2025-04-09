package com.zd.Util;

import java.util.Arrays;

public class Disk {
    static Integer DISK_HEAD_NUM = 2;
    public Integer[] disks;
    public Integer[] head = new Integer[DISK_HEAD_NUM];
    public Integer[] tagLeft;
    public Integer[] tagRight;
    public Integer[] token = new Integer[DISK_HEAD_NUM];
    public Integer[] prevToken = new Integer[DISK_HEAD_NUM];
    public Character[] prevAction = new Character[DISK_HEAD_NUM];
    public Integer diskResNum;

    public Disk(int unitNum,Integer DISK_TOKEN) {
        this.disks = new Integer[unitNum];
        this.diskResNum = unitNum;
        Arrays.fill(this.disks,-1);
        Arrays.fill(this.token,DISK_TOKEN);
        Arrays.fill(this.head,0);
        Arrays.fill(this.prevToken,0);
        Arrays.fill(this.prevAction,'#');
    }

    public Disk() {
    }
}
