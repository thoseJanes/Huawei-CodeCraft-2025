package com.zd.Service;


import com.zd.Util.DiskOperation;

import java.util.List;

public interface DiskService {
    public int[][] getDiskByExcuteDiskOperation(int[][] initDisk, int startTimeStamp, int endTimeStamp);
    public int[][] getDiskByRollBackDiskOperation(int[][] initDisk, int startTimeStamp, int endTimeStamp);

}
