package com.zd.ServiceImpl;

import com.zd.Impl.DiskImpl;
import com.zd.Impl.StorageObjectImpl;
import com.zd.Service.DiskService;
import com.zd.Util.DiskOperation;
import com.zd.Util.StorageObject;

import java.util.Arrays;
import java.util.List;

public class DiskServiceImpl implements DiskService {
    @Override
    public int[][] getDiskByExcuteDiskOperation(int[][] initDisk, int startTimeStamp, int endTimeStamp){
        DiskImpl diskImpl = new DiskImpl();
        List<DiskOperation> diskOperations = diskImpl.getDiskOperation(startTimeStamp,endTimeStamp);
        int[][] disks = new int[initDisk.length][];
        StorageObjectImpl storageObjectImpl = new StorageObjectImpl();

        disks[0] = Arrays.copyOf(initDisk[0],initDisk[0].length);
        for(int i=1;i<initDisk.length;i++){
            disks[i] = Arrays.copyOf(initDisk[i],initDisk[i].length);
        }

        for(int i=0;i<diskOperations.size();i++){
            DiskOperation diskOperation = new DiskOperation(diskOperations.get(i).objectId,diskOperations.get(i).operationType,diskOperations.get(i).timeStamp);
            StorageObject  storageObject = storageObjectImpl.getStorageObject(diskOperation.objectId,endTimeStamp);
            if(diskOperation.operationType.equals("delete")){
                for(int j=1;j<storageObject.replica.length;j++){
                    for(int k=1; k<=storageObject.size;k++){
                        disks[storageObject.replica[j]][storageObject.unit[j][k]] = 0;
                    }
                }

            }else if(diskOperation.operationType.equals("write")){
                for(int j=1;j<storageObject.replica.length;j++){
                    for(int k=1; k<=storageObject.size;k++){
                        disks[storageObject.replica[j]][storageObject.unit[j][k]] = diskOperation.objectId;
                    }
                }
            }
        }

        return disks;
    }

    @Override
    public int[][] getDiskByRollBackDiskOperation(int[][] initDisk, int startTimeStamp, int endTimeStamp){
        DiskImpl diskImpl = new DiskImpl();
        List<DiskOperation> diskOperations = diskImpl.getDiskOperation(startTimeStamp,endTimeStamp);
        int[][] disks = new int[initDisk.length][];
        StorageObjectImpl storageObjectImpl = new StorageObjectImpl();

        disks[0] = new int[initDisk[0].length];
        for(int i=1;i<initDisk.length;i++){
            disks[i] = Arrays.copyOf(initDisk[i],initDisk[i].length);
        }

        for(int i=0;i<diskOperations.size();i++){
            DiskOperation diskOperation = new DiskOperation(diskOperations.get(i).objectId,diskOperations.get(i).operationType,diskOperations.get(i).timeStamp);
            StorageObject storageObject = storageObjectImpl.getStorageObject(diskOperation.objectId,endTimeStamp);
            if(diskOperation.operationType.equals("delete")){
                for(int j=1;j<storageObject.replica.length;j++){
                    for(int k=1; k<=storageObject.size;k++){
                        disks[storageObject.replica[j]][storageObject.unit[j][k]] = diskOperation.objectId;
                    }
                }

            }else if(diskOperation.operationType.equals("write")){
                for(int j=1;j<storageObject.replica.length;j++){
                    for(int k=1; k<=storageObject.size;k++){
                        disks[storageObject.replica[j]][storageObject.unit[j][k]] = 0;
                    }
                }
            }
        }

        return disks;
    }
}
