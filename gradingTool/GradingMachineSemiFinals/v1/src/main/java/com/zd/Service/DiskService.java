package com.zd.Service;


public interface DiskService {
    public int[][] getDiskByExcuteDiskOperation(int[][] initDisk, int startTimeStamp, int endTimeStamp);
    public int[][] getDiskByRollBackDiskOperation(int[][] initDisk, int startTimeStamp, int endTimeStamp);

}
