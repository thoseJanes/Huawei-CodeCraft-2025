package com.zd.Dao;

import com.zd.Util.GeneralInformation;

public interface GeneralInformationDao {
    public int addGeneralInformation(GeneralInformation generalInfo);
    public GeneralInformation getGeneralInformationAtTimeStamp(int timeStamp);
    public void clearAll();
}
