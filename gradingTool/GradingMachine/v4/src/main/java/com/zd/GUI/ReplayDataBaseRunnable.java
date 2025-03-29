package com.zd.GUI;

import com.zd.Impl.DiskPointImpl;
import com.zd.Impl.GeneralInformationImpl;
import com.zd.Impl.StorageObjectImpl;
import com.zd.ServiceImpl.DiskServiceImpl;
import com.zd.Util.GeneralInformation;
import com.zd.Util.StorageObject;

import java.awt.*;
import java.util.List;

public class ReplayDataBaseRunnable implements Runnable{
    private int currentSlice;
    private String opera;
    private DiskDisplayPanel diskDisplayPanel;
    private DiskInfoPanel diskInfoPanel;
    private ScoreInfoPanel scoreInfoPanel;
    private DiskPointImpl diskPointImpl = new DiskPointImpl();
    private DiskServiceImpl diskServiceImpl = new DiskServiceImpl();
    private StorageObjectImpl storageObjectImpl= new StorageObjectImpl();
    private GeneralInformationImpl generalInformationImpl = new GeneralInformationImpl();

    public ReplayDataBaseRunnable() {
    }

    public ReplayDataBaseRunnable(DiskDisplayPanel diskDisplayPanel,DiskInfoPanel diskInfoPanel,ScoreInfoPanel scoreInfoPanel, int currentSlice, String opera){
        this.currentSlice = currentSlice;
        this.diskDisplayPanel = diskDisplayPanel;
        this.diskInfoPanel = diskInfoPanel;
        this.scoreInfoPanel = scoreInfoPanel;
        this.opera = opera;
    }
    @Override
    public void run() {
        try {
            EventQueue.invokeLater(()->{
                //更新diskDisplayPanel
                int[][] prevDisks = diskDisplayPanel.disks;
                int[][] currDisks;
                if(opera.equals("Prev")){
                    currDisks = diskServiceImpl.getDiskByRollBackDiskOperation(prevDisks,currentSlice,currentSlice+1);
                }else{
                    currDisks = diskServiceImpl.getDiskByExcuteDiskOperation(prevDisks,currentSlice-1, currentSlice);
                }

                int[] currDiskPoint = diskPointImpl.getDiskPointByTimestamp(currentSlice);
                List<StorageObject> storageObjects = storageObjectImpl.getStorageObjectsAtTimeStamp(currentSlice);
                diskDisplayPanel.updateParams(currDisks,storageObjects,currDiskPoint);

                //更新diskInfoPanel
                int chosenDisk = diskDisplayPanel.chosenDisk;
                int chosenUnit = diskDisplayPanel.chosenUnit;
                int chosenDiskPoint = currDiskPoint[chosenDisk];
                int chosenObjectId = currDisks[chosenDisk][chosenUnit];
                int chosenTag = storageObjects.get(chosenObjectId).tag;
                diskInfoPanel.updateParams(chosenDisk,chosenDiskPoint,chosenUnit,chosenObjectId,chosenTag,0,0);

                //更新scoreInfoPanel
                GeneralInformation generalInfo = generalInformationImpl.getGeneralInformationAtTimeStamp(currentSlice);
                scoreInfoPanel.updateParams(generalInfo.Scores,generalInfo.abortedRequests,generalInfo.finishedRequests,generalInfo.totalRequests);

            });
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
