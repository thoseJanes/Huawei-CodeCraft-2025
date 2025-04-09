package com.zd.Util;

public class TagStorage {
    public int[] tagCenters;
    public int[] tagSize;
    public int[] left;
    public int[] right;
    public int[] realLeft;
    public int[] realRight;
    public int[] resUnitNum;


    public TagStorage() {

    }

    public TagStorage(int DiskNum){
        this.tagCenters = new int[DiskNum];
        this.tagSize = new int[DiskNum];
        this.left = new int[DiskNum];
        this.right = new int[DiskNum];
        this.realLeft = new int[DiskNum];
        this.realRight = new int[DiskNum];
        this.resUnitNum = new int[DiskNum];
    }

    public TagStorage(int[] tagCenters, int[] tagSize, int[] left, int[] right) {
        this.tagCenters = tagCenters;
        this.tagSize = tagSize;
        this.left = left;
        this.right = right;
        this.resUnitNum = new int[left.length];
        for(int i=0;i<resUnitNum.length;i++){
            this.resUnitNum[i] = right[i]-left[i];
        }
    }
}
