package com.zd.Dao;

import com.zd.Util.DiskPointInfomation;

public interface DiskPointDao {
    public void addDiskPointInfo(DiskPointInfomation diskPointInfo);
    public void clearAll();
}
