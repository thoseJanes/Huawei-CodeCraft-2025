package com.zd.ServiceImpl;

import com.zd.Impl.DiskImpl;
import com.zd.Impl.DiskPointImpl;
import com.zd.Impl.GeneralInformationImpl;
import com.zd.Impl.StorageObjectImpl;
import com.zd.Util.*;

import java.util.List;

public class UpdateDataBaseRunnable implements Runnable{
    private List<DiskOperation> diskOperationsList;
    private DiskImpl diskImpl = new DiskImpl();
    private StorageObjectImpl storageObjectImpl = new StorageObjectImpl();
    private List<StorageObjectionWriteInfo> storageObjectionWriteInfoList;
    private List<StorageObjectionDeleteInfo> storageObjectionDeleteInfoList;
    private GeneralInformationImpl generalInfoImpl = new GeneralInformationImpl();
    private GeneralInformation generalInfo;
    private DiskPointImpl diskPointImpl = new DiskPointImpl();
    private DiskPointInfomation diskPointInfo;

    public UpdateDataBaseRunnable() {
    }

    public UpdateDataBaseRunnable(List<DiskOperation> diskOperationsList, List<StorageObjectionWriteInfo> storageObjectionWriteInfoList, List<StorageObjectionDeleteInfo> storageObjectionDeleteInfoList, GeneralInformation generalInfo, DiskPointInfomation diskPointInfo) {
        this.diskOperationsList = diskOperationsList;
        this.storageObjectionWriteInfoList = storageObjectionWriteInfoList;
        this.storageObjectionDeleteInfoList = storageObjectionDeleteInfoList;
        this.generalInfo = generalInfo;
        this.diskPointInfo = diskPointInfo;
    }

    @Override
    public void run() {
        System.out.println("数据库每帧更新线程启动"); // 确认线程执行
        //磁盘操作写入数据库
        for(DiskOperation diskOperation : diskOperationsList){
            if(diskOperation.operationType.equals("delete")){
                diskImpl.addDiskDeleteInfo(diskOperation.timeStamp,diskOperation.objectId);
            }else if(diskOperation.operationType.equals("write")){
                diskImpl.addDiskWriteInfo(diskOperation.timeStamp,diskOperation.objectId);
            }
        }

        //数据库对象写入数据库
        for(StorageObjectionWriteInfo objWriteInfo : storageObjectionWriteInfoList){
            storageObjectImpl.addStorageObject(objWriteInfo.objectId,objWriteInfo.storageObjectl,objWriteInfo.writeTimeStamp);
        }

        for(StorageObjectionDeleteInfo objDeleteInfo : storageObjectionDeleteInfoList){
            storageObjectImpl.updateStorageObjectDeleteInfo(objDeleteInfo.objectId,objDeleteInfo.deleteTimeStamp);
        }

        //总信息写入数据库
        generalInfoImpl.addGeneralInformation(generalInfo);

        //磁头信息写入数据库
        diskPointImpl.addDiskPointInfo(diskPointInfo);
        System.out.println("数据库更新线程结束，数据库已更新完成！"); // 确认线程执行结束

    }
}
